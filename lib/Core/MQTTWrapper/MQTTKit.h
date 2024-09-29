/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2023 Uniot <contact@uniot.io>
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
class MQTTKit : public ISchedulerConnectionKit, public CoreEventListener {
  typedef std::function<void(CBORObject &)> CBORExtender;
  friend class MQTTDevice;

 public:
  enum Topic { CONNECTION = FOURCC(mqtt) };
  enum Msg {
    FAILED = 0,
    SUCCESS
  };

  MQTTKit(const Credentials &credentials, CBORExtender infoExtender = nullptr)
      : mpCredentials(&credentials),
        mPath(credentials),
        mInfoExtender(infoExtender),
        mPubSubClient(mWiFiClient),
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

    // mWiFiClient.allowSelfSignedCerts();
    // mWiFiClient.setInsecure();
  }

  ~MQTTKit() {
    CoreEventListener::stopListeningToEvent(NetworkScheduler::Topic::CONNECTION);
    // TODO: implement, remove all devices
  }

  void setServer(const char *domain, uint16_t port) {
    mPubSubClient.setServer(domain, port);
  }

  void addDevice(MQTTDevice &device) {
    if (mDevices.pushUnique(&device)) {
      device.kit(this);
      device.topics()->forEach([this](String topic) {
        mPubSubClient.subscribe(topic.c_str());
      });
    }
  }

  void removeDevice(MQTTDevice &device) {
    if (mDevices.removeOne(&device)) {
      device.kit(nullptr);
      device.topics()->forEach([this](String topic) {
        mPubSubClient.unsubscribe(topic.c_str());
      });
    }
  }

  const MQTTPath &getPath() {
    return mPath;
  }

  void renewSubscriptions() {
    mDevices.forEach([this](MQTTDevice *device) {
      device->unsubscribeFromAll();
      device->syncSubscriptions();
    });
  }

  virtual void pushTo(TaskScheduler &scheduler) override {
    scheduler.push("mqtt", mTaskMQTT);
  }

  virtual void attach() override {}

  virtual void onEventReceived(unsigned int topic, int msg) override {
    if (NetworkScheduler::CONNECTION == topic) {
      switch (msg) {
        case NetworkScheduler::SUCCESS:
          mTaskMQTT->attach(10);
          break;
        case NetworkScheduler::ACCESS_POINT:
        case NetworkScheduler::CONNECTING:
        case NetworkScheduler::DISCONNECTED:
        case NetworkScheduler::FAILED:
        default:
          mTaskMQTT->detach();
          break;
      }
    }
  }

 protected:
  PubSubClient *client() {
    return &mPubSubClient;
  }

 private:
  inline void _initTasks() {
    mTaskMQTT = TaskScheduler::make([this](SchedulerTask &self, short t) {
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

  Bytes _buildCOSEMessage(const Bytes &payload, bool sign = false) {
    COSEMessage obj;
    obj.setPayload(payload);
    if (sign) {
      obj.sign(*mpCredentials);
    }
    return obj.build();
  }

  bool _readCOSEMessage(const Bytes &message, Bytes &outPayload) {
    COSEMessage obj(message);
    if (obj.wasReadSuccessful()) {
      outPayload = obj.getPayload();
      return true;
    }
    return false;
  }

  void _prepareOnlinePacket(CBORObject &packet) {
    packet
        .put("online", 1)
        .put("connection_id", mConnectionId++);
  }

  void _prepareOfflinePacket(CBORObject &packet) {
    packet
        .put("online", 0)
        .put("connection_id", mConnectionId);
  }

  String _getClientId() {
    return "device:" + mpCredentials->getDeviceId();  // TODO: owner
  }

  String _getUserLogin() {
    return mpCredentials->getPublicKey();
  }

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

  const Credentials *mpCredentials;

  MQTTPath mPath;
  CBORExtender mInfoExtender;
  PubSubClient mPubSubClient;

  int mConnectionId;

  WiFiClient mWiFiClient;
  // WiFiClientSecure mWiFiClient;
  ClearQueue<MQTTDevice *> mDevices;
  TaskScheduler::TaskPtr mTaskMQTT;
};
}  // namespace uniot
