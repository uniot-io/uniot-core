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

#include "MQTTDevice.h"

#include <Arduino.h>
#include <Logger.h>

#include "MQTTKit.h"

namespace uniot {
MQTTDevice::~MQTTDevice() {
  if (mpKit) {
    mpKit->removeDevice(this);
    mpKit = nullptr;
  }
}

void MQTTDevice::unsubscribeFromAll() {
  while (!mTopics.isEmpty()) {
    auto topic = mTopics.hardPop();
    if (mpKit) {
      mpKit->client()->unsubscribe(topic.c_str());
    }
  }
}

const String& MQTTDevice::subscribe(const String &topic) {
  mTopics.push(topic);
  if (mpKit) {
    mpKit->client()->subscribe(topic.c_str());
  }
  return topic;
}

String MQTTDevice::subscribeDevice(const String &subTopic) {
  if (mpKit) {
    return subscribe(mpKit->getPath().buildDevicePath(subTopic));
  } else {
    UNIOT_LOG_WARN("use detailed subscription after adding device to kit");
  }
  return {};
}

String MQTTDevice::subscribeGroup(const String &groupId, const String &subTopic) {
  if (mpKit) {
    return subscribe(mpKit->getPath().buildGroupPath(groupId, subTopic));
  } else {
    UNIOT_LOG_WARN("use detailed subscription after adding device to kit");
  }
  return {};
}

void MQTTDevice::publish(const String &topic, const Bytes &payload, bool retained) {
  if (mpKit) {
    mpKit->client()->publish(topic.c_str(), payload.raw(), payload.size(), retained);
  }
}

void MQTTDevice::publishDevice(const String &subTopic, const Bytes &payload, bool retained) {
  if (mpKit) {
    publish(mpKit->getPath().buildDevicePath(subTopic), payload, retained);
  }
}

void MQTTDevice::publishGroup(const String &groupId, const String &subTopic, const Bytes &payload, bool retained) {
  if (mpKit) {
    publish(mpKit->getPath().buildGroupPath(groupId, subTopic), payload, retained);
  }
}

bool MQTTDevice::isSubscribed(const String &topic) {
  mTopics.begin();
  while (!mTopics.isEnd()) {
    if (isTopicMatch(mTopics.current(), topic)) {
      return true;
    }
    mTopics.next();
  }
  return false;
}

bool MQTTDevice::isTopicMatch(const String &storedTopic, const String &incomingTopic) const {
  unsigned int storedPos = 0, incomingPos = 0;

  while (storedPos < storedTopic.length() && incomingPos < incomingTopic.length()) {
    auto storedSlashPos = storedTopic.indexOf('/', storedPos);
    auto incomingSlashPos = incomingTopic.indexOf('/', incomingPos);

    if (storedPos < storedTopic.length() && storedTopic[storedPos] == '#') {
      return true;
    }

    // If no separator found, set to end of string
    if (storedSlashPos == -1) {
      storedSlashPos = storedTopic.length();
    }
    if (incomingSlashPos == -1) {
      incomingSlashPos = incomingTopic.length();
    }

    auto storedSegment = storedTopic.substring(storedPos, storedSlashPos);
    auto incomingSegment = incomingTopic.substring(incomingPos, incomingSlashPos);

    if (!(storedSegment == "+" || storedSegment == incomingSegment /* || storedSegment == "" */)) {
      break;
    }

    storedPos = storedSlashPos + 1;
    incomingPos = incomingSlashPos + 1;
  }

  // Check if both topics have been fully traversed
  return storedPos >= storedTopic.length() && incomingPos >= incomingTopic.length();
}
}  // namespace uniot