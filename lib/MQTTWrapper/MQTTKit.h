/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2020 Uniot <info.uniot@gmail.com>
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
#include "MQTTDevice.h"

namespace uniot
{
class MQTTKit : public IExecutor, public GeneralPublisher
{
  friend class MQTTDevice;

public:
  enum Topic { CONNECTION = FOURCC(mqtt) };
  enum Msg { FAILED = 0, SUCCESS };

  MQTTKit() : mPubSubClient(mWiFiClient), mConnectionId(random(0xffff)), mClientId(random(0xffffff))
  {
    mPubSubClient.setCallback([this](char *topic, uint8_t *payload, unsigned int length) {
      mDevices.forEach([&](MQTTDevice *device) {
        MQTTDevice::Handler callbackHandler = device->handler();
        if (callbackHandler && device->isSubscribed(String(topic)))
        {
          callbackHandler(topic, Bytes(payload, length));
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
      if (mPubSubClient.connect(String(mClientId, HEX).c_str(), "service", 0, true, (String("disconnected ") + String(mConnectionId)).c_str()))
      {
        mPubSubClient.publish("service", (String("connected ") + String(mConnectionId++)).c_str(), true); // publish an announcement
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
  }

protected:
  PubSubClient *client()
  {
    return &mPubSubClient;
  }

private:
  PubSubClient mPubSubClient;

  long mConnectionId;
  long mClientId;

  WiFiClient mWiFiClient;
  ClearQueue<MQTTDevice *> mDevices;
};
} // namespace uniot