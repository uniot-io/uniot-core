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

#include <Arduino.h>
#include <Logger.h>
#include "MQTTKit.h"
#include "MQTTDevice.h"

namespace uniot
{
MQTTDevice::~MQTTDevice()
{
  if (mpKit)
  {
    mpKit->removeDevice(this);
    mpKit = nullptr;
  }
}

void MQTTDevice::subscribe(const String &topic)
{
  mTopics.push(topic);
  if (mpKit)
  {
    mpKit->client()->subscribe(topic.c_str());
  }
}

void MQTTDevice::subscribeDevice(const String &subTopic)
{
  if (mpKit)
    subscribe(mpKit->getPath().buildDevicePath(subTopic));
  else
    UNIOT_LOG_WARN("use detailed subscription after adding device to kit");
}

void MQTTDevice::subscribeGroup(const String &subTopic)
{
  if (mpKit)
    subscribe(mpKit->getPath().buildGroupPath(subTopic));
  else
    UNIOT_LOG_WARN("use detailed subscription after adding device to kit");
}

void MQTTDevice::publish(const String &topic, const Bytes &payload, bool retained)
{
  if (mpKit)
  {
    mpKit->client()->publish(topic.c_str(), payload.raw(), payload.size(), retained);
  }
}

void MQTTDevice::publishDevice(const String &subTopic, const Bytes &payload, bool retained)
{
  if (mpKit)
    publish(mpKit->getPath().buildDevicePath(subTopic), payload, retained);
}

void MQTTDevice::publishGroup(const String &subTopic, const Bytes &payload, bool retained)
{
  if (mpKit)
    publish(mpKit->getPath().buildGroupPath(subTopic), payload, retained);
}

bool MQTTDevice::isSubscribed(const String &topic)
{
  return mTopics.contains(topic);
}
} // namespace uniot