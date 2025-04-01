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
#include <NetworkScheduler.h>
#include <PubSubClient.h>
#include <TaskScheduler.h>

#include <functional>

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
   * @enum Topic
   * @brief Event topics this class can emit
   */
  enum Topic {
    CONNECTION = FOURCC(mqtt) /**< MQTT connection topic (value from FOURCC("mqtt")) */
  };

  /**
   * @enum Msg
   * @brief Message types for the CONNECTION topic
   */
  enum Msg {
    FAILED = 0,       /**< Connection failed */
    SUCCESS = 1       /**< Connection successful */
  };

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
        mConnectionId(0) {
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
    CoreEventListener::listenToEvent(NetworkScheduler::Topic::CONNECTION);
    CoreEventListener::listenToEvent(Date::Topic::TIME);

    // mWiFiClient.allowSelfSignedCerts();
    // mWiFiClient.setInsecure();
  }

  /**
   * @brief Destructor - cleans up event listeners
   */
  ~MQTTKit() {
    CoreEventListener::stopListeningToEvent(NetworkScheduler::Topic::CONNECTION);
    CoreEventListener::stopListeningToEvent(Date::Topic::TIME);
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
    if (NetworkScheduler::CONNECTION == topic) {
      switch (msg) {
        case NetworkScheduler::SUCCESS:
          mNetworkConnected = true;
          Date::getInstance().forceSync();
          break;
        case NetworkScheduler::ACCESS_POINT:
        case NetworkScheduler::CONNECTING:
        case NetworkScheduler::DISCONNECTED:
        case NetworkScheduler::FAILED:
        default:
          mNetworkConnected = false;
          mTaskMQTT->detach();
          break;
      }
      return;
    }
    if (Date::TIME == topic) {
      switch (msg) {
        case Date::SYNCED:
          if (!mTaskMQTT->isAttached()) {
            mTaskMQTT->attach(10);
          }
          break;
        default:
          break;
      }
      return;
    }
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
    mTaskMQTT = TaskScheduler::make([this](SchedulerTask &self, short t) {
      if (!mNetworkConnected) {
        UNIOT_LOG_DEBUG("MQTT: Network is not connected");
        return;
      }
      if (!mPubSubClient.connected()) {
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
          CoreEventEmitter::emitEvent(Topic::CONNECTION, Msg::SUCCESS);
        } else {
          CoreEventEmitter::emitEvent(Topic::CONNECTION, Msg::FAILED);
        }
      }
      mPubSubClient.loop();
    });
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
    auto kid = mpCredentials->keyId();  // NOTE: dynamic data must be within the scope of the obj.build() function
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
    return "device:" + mpCredentials->getDeviceId();  // TODO: owner
  }

  /**
   * @brief Generates the MQTT user login
   * @retval publicKey The public key string
   */
  String _getUserLogin() {
    return mpCredentials->getPublicKey();
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
    CBORObject password;
    auto protectedData = password.putMap("protected");
    protectedData.put("device", mpCredentials->getDeviceId().c_str());
    protectedData.put("owner", mpCredentials->getOwnerId().c_str());
    protectedData.put("creator", mpCredentials->getCreatorId().c_str());
    protectedData.put("timestamp", static_cast<int64_t>(Date::now()));
    auto unprotectedData = password.putMap("unprotected");
    unprotectedData.put("alg", "EdDSA");

    auto signature = mpCredentials->sign(protectedData.build());
    password.put("signature", signature.raw(), signature.size());

    return password.build();
  }

  const Credentials *mpCredentials;  /**< Device credentials */

  MQTTPath mPath;                   /**< Helper for building MQTT paths */
  CBORExtender mInfoExtender;       /**< Callback for extending status info */
  PubSubClient mPubSubClient;       /**< MQTT client implementation */

  bool mNetworkConnected;           /**< Network connection status */
  int mConnectionId;                /**< Current connection sequence number */

  WiFiClient mWiFiClient;           /**< TCP client for MQTT communication */
  // WiFiClientSecure mWiFiClient;  /**< Secure TCP client (commented out) */
  ClearQueue<MQTTDevice *> mDevices; /**< List of managed MQTT devices */
  TaskScheduler::TaskPtr mTaskMQTT;  /**< MQTT maintenance task */
};
/** @} */
}  // namespace uniot
