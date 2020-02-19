/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2020 Uniot <contact@uniot.io>
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

#include <Common.h>
#include <IExecutor.h>
#include <Publisher.h>
#include <ClearQueue.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Bytes.h>
#include <CBOR.h>
#include "MQTTDevice.h"
#include "CallbackMQTTDevice.h"
#include "MQTTPath.h"

namespace uniot
{
class MQTTKit : public IExecutor, public GeneralPublisher
{
  friend class MQTTDevice;

public:
  enum Topic { CONNECTION = FOURCC(mqtt) };
  enum Msg { FAILED = 0, SUCCESS };

  MQTTKit(const Credentials &credentials)
      : mPath(credentials),
        mPubSubClient(mWiFiClient),
        mConnectionId(0),
        mClientId(credentials.getShortDeviceId())
  {
    mPubSubClient.setCallback([this](char *topic, uint8_t *payload, unsigned int length) {
      mDevices.forEach([&](MQTTDevice *device) {
        if (device->isSubscribed(String(topic)))
        {
          device->handle(topic, Bytes(payload, length));
        }
      });
    });
  }

  void setServer(const char *domain, uint16_t port)
  {
    mPubSubClient.setServer(domain, port);
  }

  void addDevice(MQTTDevice *device)
  {
    if (mDevices.pushUnique(device))
    {
      device->kit(this);
      device->topics()->forEach([this](String topic) {
        mPubSubClient.subscribe(topic.c_str());
      });
    }
  }

  void removeDevice(MQTTDevice *device)
  {
    if (mDevices.removeOne(device))
    {
      device->kit(nullptr);
      device->topics()->forEach([this](String topic) {
        mPubSubClient.unsubscribe(topic.c_str());
      });
    }
  }

  virtual uint8_t execute() override
  {
    if (!mPubSubClient.connected())
    {
      Serial.print("Attempting MQTT connection...    ");
      Serial.println(mConnectionId);
      auto offlinePacket = CBOR()
                  .put("state", 0)
                  .put("id", mConnectionId)
                  .build();
      if (mPubSubClient.connect(
              mPath.getCredentials()->getDeviceId().c_str(),
              mPath.buildDevicePath("online").c_str(),
              0,
              true,
              offlinePacket.raw(),
              offlinePacket.size()))
      {
        auto onlinePacket = CBOR()
                .put("state", 1)
                .put("id", mConnectionId++)
                .build();
        mPubSubClient.publish(
            mPath.buildDevicePath("online").c_str(),
            onlinePacket.raw(),
            onlinePacket.size(),
            true); // publish an announcement
        mDevices.forEach([this](MQTTDevice *device) {
          device->topics()->forEach([this](String topic) {
            mPubSubClient.subscribe(topic.c_str());
          });
        });
        publish(Topic::CONNECTION, Msg::SUCCESS);
      }
      else
      {
        publish(Topic::CONNECTION, Msg::FAILED);
      }
    }
    mPubSubClient.loop();
    return 0;
  }

protected:
  PubSubClient *client()
  {
    return &mPubSubClient;
  }

private:
  MQTTPath mPath;
  PubSubClient mPubSubClient;

  long mConnectionId;
  long mClientId;

  WiFiClient mWiFiClient;
  ClearQueue<MQTTDevice *> mDevices;
};
} // namespace uniot