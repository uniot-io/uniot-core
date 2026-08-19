/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2024 Uniot <contact@uniot.io>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @cond */
/**
 * DO NOT DELETE THE "networking" GROUP DEFINITION BELOW.
 * Used to create the Networking topic in the documentation. If you want to delete this file,
 * please paste the group definition into another file and delete this one.
 */
/** @endcond */

/**
 * @defgroup networking Networking
 * @brief Networking and communication classes for the Uniot project
 */

/**
 * @defgroup mqtt_communication MQTT Communication
 * @ingroup networking
 */

#pragma once

#if defined(ESP8266)
#include "ESP8266WiFi.h"
#elif defined(ESP32)
#include "WiFi.h"
#endif

#include <Bytes.h>
#include <CBORObject.h>
#include <COSEMessage.h>
#include <ClearQueue.h>
#include <Common.h>
#include <Date.h>
#include <EventListener.h>
#include <NetworkEvents.h>
#include <NetworkScheduler.h>
#include <MQTTEvents.h>
#include <PubSubClient.h>
#include <TaskScheduler.h>

#include <atomic>
#include <functional>

#if defined(ESP32)
#include <lwip/tcp.h>
#include <sys/socket.h>
#endif

#include "CallbackMQTTDevice.h"
#include "MQTTDevice.h"
#include "MQTTPath.h"

namespace uniot {
/**
 * @brief MQTT communication wrapper that manages devices and their subscriptions
 * @defgroup mqtt_kit MQTT Kit
 * @ingroup mqtt_communication
 *
 * This class handles MQTT connections, message processing, and device management.
 * It implements ISchedulerConnectionKit for integration with the task scheduler system
 * and inherits from CoreEventListener to handle network and time-related events.
 * @{
 */
class MQTTKit : public ISchedulerConnectionKit, public CoreEventListener {
  /** @brief Function type for extending CBOR objects with additional data */
  typedef std::function<void(CBORObject &)> CBORExtender;
  friend class MQTTDevice;

 public:
  /**
   * @brief Constructs an MQTTKit instance
   * @param credentials The credentials to use for MQTT authentication
   * @param infoExtender Optional callback to extend status messages with additional data
   */
  MQTTKit(const Credentials &credentials, CBORExtender infoExtender = nullptr)
      : mpCredentials(&credentials),
        mPath(credentials),
        mInfoExtender(infoExtender),
        mPubSubClient(mWiFiClient),
        mNetworkConnected(false),
        mConnectionId(0),
        mMqttConnected(false) {
    mPubSubClient.setCallback([this](char *topic, uint8_t *payload, unsigned int length) {
      mDevices.forEach([&](MQTTDevice *device) {
        if (device->isSubscribed(String(topic))) {
          if (!length) {
            device->handle(topic, Bytes());
            return;
          }

          Bytes decoded;
          if (_readCOSEMessage(Bytes(payload, length), decoded)) {
            device->handle(topic, decoded);
          } else {
            UNIOT_LOG_ERROR("Failed to decode message on topic: %s", topic);
          }
        }
      });
    });
    _initTasks();
    CoreEventListener::listenToEvent(events::network::Topic::CONNECTION);
    CoreEventListener::listenToEvent(events::date::Topic::TIME);

    // mWiFiClient.allowSelfSignedCerts();
    // mWiFiClient.setInsecure();
  }

  /**
   * @brief Destructor - cleans up event listeners
   */
  ~MQTTKit() {
    CoreEventListener::stopListeningToEvent(events::network::Topic::CONNECTION);
    CoreEventListener::stopListeningToEvent(events::date::Topic::TIME);
    // TODO: implement, remove all devices
  }

  /**
   * @brief Sets the MQTT broker server address and port
   * @param domain The server domain name or IP address
   * @param port The server port
   */
  void setServer(const char *domain, uint16_t port) {
    mPubSubClient.setServer(domain, port);
  }

  /**
   * @brief Adds a device to be managed by this MQTT kit
   *
   * The device will be initialized with this kit and its topics will be subscribed
   *
   * @param device The device to add
   */
  void addDevice(MQTTDevice &device) {
    if (mDevices.pushUnique(&device)) {
      device.kit(this);
      device.topics()->forEach([this](String topic) {
        mPubSubClient.subscribe(topic.c_str());
      });
    }
  }

  /**
   * @brief Removes a device from this MQTT kit
   *
   * The device will be detached from this kit and its topic subscriptions will be removed
   *
   * @param device The device to remove
   */
  void removeDevice(MQTTDevice &device) {
    if (mDevices.removeOne(&device)) {
      device.kit(nullptr);
      device.topics()->forEach([this](String topic) {
        mPubSubClient.unsubscribe(topic.c_str());
      });
    }
  }

  /**
   * @brief Gets the MQTT path helper object
   * @retval mPath The MQTT path helper object
   */
  const MQTTPath &getPath() {
    return mPath;
  }

  /**
   * @brief Renews all device subscriptions
   *
   * Unsubscribes from all topics and then resubscribes to ensure
   * subscriptions are current
   */
  void renewSubscriptions() {
    mDevices.forEach([this](MQTTDevice *device) {
      device->unsubscribeFromAll();
      device->syncSubscriptions();
    });
  }

  /**
   * @brief Registers MQTT tasks with the provided scheduler
   * @param scheduler The scheduler to register tasks with
   * @implements ISchedulerConnectionKit
   */
  virtual void pushTo(TaskScheduler &scheduler) override {
    scheduler.push("mqtt", mTaskMQTT);
    scheduler.push("ntp_retry", mTaskNtpRetry);
  }

  /**
   * @brief Attaches this kit (empty implementation)
   * @implements ISchedulerConnectionKit
   */
  virtual void attach() override {}

  /**
   * @brief Handles network and time events
   *
   * Handles network connection events to enable/disable MQTT connections
   * and time synchronization events to initialize MQTT tasks
   *
   * @param topic The event topic
   * @param msg The event message
   * @implements CoreEventListener
   */
  virtual void onEventReceived(unsigned int topic, int msg) override {
    if (events::network::Topic::CONNECTION == topic) {
      switch (msg) {
        case events::network::Msg::SUCCESS:
          mNetworkConnected = true;
          // forceSync() blocks for up to ~2 s. Run it from a scheduler task
          // rather than here, so the event bus dispatch is not stalled.
          if (!mTaskNtpRetry->isAttached()) {
            mTaskNtpRetry->once(1);
          }
          break;
        case events::network::Msg::ACCESS_POINT:
        case events::network::Msg::AVAILABLE:
        case events::network::Msg::CONNECTING:
        case events::network::Msg::DISCONNECTING:
        case events::network::Msg::DISCONNECTED:
        case events::network::Msg::FAILED:
        default:
          mNetworkConnected = false;
          mMqttConnected.store(false);
          mTaskMQTT->detach();
          mTaskNtpRetry->detach();
          break;
      }
      return;
    }
    if (events::date::Topic::TIME == topic) {
      switch (msg) {
        case events::date::Msg::SYNCED:
          mTaskNtpRetry->detach();
          if (!mTaskMQTT->isAttached()) {
            mTaskMQTT->attach(10);
          }
          break;
        case events::date::Msg::SYNC_FAILED:
          // MQTT cannot start without a valid clock, so keep retrying.
          if (!mTaskNtpRetry->isAttached()) {
            mTaskNtpRetry->once(3000);
          }
          break;
        default:
          break;
      }
      return;
    }
  }

  /**
   * @brief Returns true if the MQTT broker connection is currently established
   *
   * Thread-safe: reads an atomic flag written by the MQTT task on connect and
   * disconnect. Safe to call from any FreeRTOS task.
   * @retval bool true if connected to the broker
   */
  bool isMqttConnected() const { return mMqttConnected.load(); }

  /**
   * @brief Publishes the retained offline status and closes the connection
   *
   * Use before a deliberate restart so the device does not appear to the
   * broker as having crashed.
   */
  void forceDisconnect() {
    Bytes packetExtention;
    if (mInfoExtender) {
      CBORObject packet;
      mInfoExtender(packet);
      packetExtention = packet.build();
    }
    CBORObject offlineCBOR(packetExtention);
    _prepareOfflinePacket(offlineCBOR);
    auto offlinePacket = _buildCOSEMessage(offlineCBOR.build());
    mPubSubClient.publish(
        mPath.buildDevicePath("status").c_str(),
        offlinePacket.raw(),
        offlinePacket.size(),
        true);
    mPubSubClient.disconnect();
    mMqttConnected.store(false);
  }

 protected:
  /**
   * @brief Gets access to the underlying PubSubClient
   * @retval mPubSubClient The underlying PubSubClient instance
   */
  PubSubClient *client() {
    return &mPubSubClient;
  }

 private:
  /**
   * @brief Initializes MQTT connection and maintenance tasks
   */
  inline void _initTasks() {
    mTaskNtpRetry = TaskScheduler::make([this](SchedulerTask &self, short t) {
      // Active only until NTP succeeds; detached on TIME/SYNCED. forceSync() is
      // blocking, which a scheduler tick tolerates but the event bus does not.
      if (mNetworkConnected) {
        UNIOT_LOG_DEBUG("NTP retry: attempting forceSync");
        Date::getInstance().forceSync();
      }
    });

    mTaskMQTT = TaskScheduler::make([this](SchedulerTask &self, short t) {
      if (!mNetworkConnected) {
        UNIOT_LOG_DEBUG("MQTT: Network is not connected");
        return;
      }
      if (!mPubSubClient.connected()) {
        mMqttConnected.store(false);
        UNIOT_LOG_DEBUG("Attempting MQTT connection #%d...", mConnectionId);
        Bytes packetExtention;
        if (mInfoExtender) {
          CBORObject packet;
          mInfoExtender(packet);
          packetExtention = packet.build();
        }

        CBORObject offlineCBOR(packetExtention);
        _prepareOfflinePacket(offlineCBOR);
        auto offlinePacket = _buildCOSEMessage(offlineCBOR.build());
        auto password = _getUserPassword();
        if (mPubSubClient.connect(
                _getClientId().c_str(),
                _getUserLogin().c_str(),
                (const char *)password.raw(),
                password.size(),
                mPath.buildDevicePath("status").c_str(),
                0,
                true,
                (const char *)offlinePacket.raw(),
                offlinePacket.size(),
                true)) {
          CBORObject onlineCBOR(packetExtention);
          _prepareOnlinePacket(onlineCBOR);
          auto onlinePacket = _buildCOSEMessage(onlineCBOR.build());
          mPubSubClient.publish(
              mPath.buildDevicePath("status").c_str(),
              onlinePacket.raw(),
              onlinePacket.size(),
              true);  // publish an announcement
          mDevices.forEach([this](MQTTDevice *device) {
            device->topics()->forEach([this](String topic) {
              mPubSubClient.subscribe(topic.c_str());
            });
          });
          _applyTcpKeepalive();
          mMqttConnected.store(true);
          CoreEventEmitter::emitEvent(events::mqtt::Topic::CONNECTION, events::mqtt::Msg::SUCCESS);
        } else {
          mMqttConnected.store(false);
          CoreEventEmitter::emitEvent(events::mqtt::Topic::CONNECTION, events::mqtt::Msg::FAILED);
        }
      }
      mPubSubClient.loop();
    });
  }

  /**
   * @brief Applies TCP keepalive options to the active MQTT socket
   *
   * Called once per connection, immediately after a successful connect().
   * Without it, a transient NAT or carrier outage can leave the TCP connection
   * half-open for 75-90 seconds before lwIP gives up. No-op outside ESP32.
   */
  void _applyTcpKeepalive() {
#if defined(ESP32)
    int fd = mWiFiClient.fd();
    if (fd < 0) {
      UNIOT_LOG_WARN("MQTT: TCP keepalive skipped, invalid socket fd");
      return;
    }
    static constexpr int kEnable = 1;
    static constexpr int kIdle = 10;   // seconds before the first probe; must be < MQTT_KEEPALIVE
    static constexpr int kIntvl = 5;   // seconds between probes
    static constexpr int kCount = 3;   // probes before giving up
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &kEnable, sizeof(kEnable));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &kIdle, sizeof(kIdle));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &kIntvl, sizeof(kIntvl));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &kCount, sizeof(kCount));
    UNIOT_LOG_DEBUG("MQTT: TCP keepalive applied (idle=%ds, intvl=%ds, cnt=%d)", kIdle, kIntvl, kCount);
#endif
  }

  /**
   * @brief Creates a COSE message from payload
   * @param payload The payload to encapsulate
   * @param sign Whether to sign the message
   * @retval Bytes COSE message bytes
   */
  Bytes _buildCOSEMessage(const Bytes &payload, bool sign = false) {
    COSEMessage obj;
    obj.setPayload(payload);
    auto kid = mpCredentials->keyId();  // FIXME: dynamic data must be within the scope of the obj.build() function
    if (sign) {
      obj.sign(*mpCredentials);
      obj.setUnprotectedKid(kid);
    }
    return obj.build();
  }

  /**
   * @brief Decodes a COSE message into its payload
   * @param message The COSE message to decode
   * @param outPayload The output buffer for the decoded payload
   * @retval true Decoding was successful
   * @retval false Decoding failed
   */
  bool _readCOSEMessage(const Bytes &message, Bytes &outPayload) {
    COSEMessage obj(message);
    if (obj.wasReadSuccessful()) {
      outPayload = obj.getPayload();
      return true;
    }
    return false;
  }

  /**
   * @brief Prepares the online status packet
   *
   * Adds online status and increments connection ID
   *
   * @param packet The CBOR object to populate
   */
  void _prepareOnlinePacket(CBORObject &packet) {
    packet
        .put("online", 1)
        .put("connection_id", mConnectionId++);
  }

  /**
   * @brief Prepares the offline status packet
   *
   * Sets offline status and maintains current connection ID
   *
   * @param packet The CBOR object to populate
   */
  void _prepareOfflinePacket(CBORObject &packet) {
    packet
        .put("online", 0)
        .put("connection_id", mConnectionId);
  }

  /**
   * @brief Generates the MQTT client ID
   * @retval id The client ID string
   */
  String _getClientId() {
    return _getUserLogin();
  }

  /**
   * @brief Generates the MQTT user login
   * @retval publicKey The public key string
   */
  String _getUserLogin() {
    return "device:" + mpCredentials->getOwnerId() + "@" + mpCredentials->getDeviceId();
  }

  /**
   * @brief Generates the MQTT password as a signed CBOR object
   *
   * Creates a signed password object containing device ID, owner ID,
   * creator ID, and timestamp
   *
   * @retval password The signed password object
   */
  Bytes _getUserPassword() {
    CBORObject claims;
    claims.put("device", mpCredentials->getDeviceId().c_str());
    claims.put("owner", mpCredentials->getOwnerId().c_str());
    claims.put("creator", mpCredentials->getCreatorId().c_str());
    claims.put("timestamp", static_cast<int64_t>(Date::now()));

    COSEMessage password;
    auto kid = mpCredentials->keyId();  // FIXME: dynamic data must be within the scope of the obj.build() function
    password.setUnprotectedKid(kid);
    password.setPayload(claims.build());
    password.sign(*mpCredentials);
    return password.build();
  }

  const Credentials *mpCredentials;  /**< Device credentials */

  MQTTPath mPath;                   /**< Helper for building MQTT paths */
  CBORExtender mInfoExtender;       /**< Callback for extending status info */
  PubSubClient mPubSubClient;       /**< MQTT client implementation */

  bool mNetworkConnected;           /**< Network connection status */
  int mConnectionId;                /**< Current connection sequence number */
  std::atomic<bool> mMqttConnected; /**< True while the broker connection is established */

  WiFiClient mWiFiClient;           /**< TCP client for MQTT communication */
  // WiFiClientSecure mWiFiClient;  /**< Secure TCP client (commented out) */
  ClearQueue<MQTTDevice *> mDevices;    /**< List of managed MQTT devices */
  TaskScheduler::TaskPtr mTaskMQTT;     /**< MQTT maintenance task */
  TaskScheduler::TaskPtr mTaskNtpRetry; /**< NTP retry task, active only until time is synced */
};
/** @} */
}  // namespace uniot
