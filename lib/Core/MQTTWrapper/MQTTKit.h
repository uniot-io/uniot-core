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

#include <Bytes.h>
#include <CBORObject.h>
#include <ClearQueue.h>
#include <Common.h>
#include <Date.h>
#include <ESP8266WiFi.h>
#include <EventEmitter.h>
#include <IExecutor.h>
#include <PubSubClient.h>

#include <functional>

#include "CallbackMQTTDevice.h"
#include "MQTTDevice.h"
#include "MQTTPath.h"

namespace uniot {
class MQTTKit : public IExecutor, public CoreEventEmitter {
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
          device->handle(topic, Bytes(payload, length));
        }
      });
    });

    // mWiFiClient.allowSelfSignedCerts();
    // mWiFiClient.setInsecure();
  }

  ~MQTTKit() {
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

  virtual uint8_t execute() override {
    if (!mPubSubClient.connected()) {
      Serial.print("Attempting MQTT connection...    ");
      Serial.println(mConnectionId);
      CBORObject offlineCBOR;
      _prepareOfflinePacket(offlineCBOR);
      auto offlinePacket = offlineCBOR.build();
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
        CBORObject onlineCBOR;
        _prepareOnlinePacket(onlineCBOR);
        auto onlinePacket = onlineCBOR.build();
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
    return 0;
  }

 protected:
  PubSubClient *client() {
    return &mPubSubClient;
  }

 private:
  void _prepareOnlinePacket(CBORObject &packet) {
    packet
        .put("online", 1)
        .put("connection_id", mConnectionId++);

    if (mInfoExtender)
      mInfoExtender(packet);
  }

  void _prepareOfflinePacket(CBORObject &packet) {
    packet
        .put("online", 0)
        .put("connection_id", mConnectionId);

    if (mInfoExtender)
      mInfoExtender(packet);
  }

  String _getClientId() {
    return "device:" + mpCredentials->getDeviceId(); // TODO: owner
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
    protectedData.put("timestamp", Date::now());
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
};
}  // namespace uniot
