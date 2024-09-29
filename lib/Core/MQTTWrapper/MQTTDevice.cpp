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
    mpKit->removeDevice(*this);
    mpKit = nullptr;
  }
}

String MQTTDevice::getDeviceId() const {
  if (mpKit) {
    return mpKit->getPath().getDeviceId();
  } else {
    UNIOT_LOG_WARN("getting device id before adding device to kit");
  }
  return {};
}

String MQTTDevice::getOwnerId() const {
  if (mpKit) {
    return mpKit->getPath().getOwnerId();
  } else {
    UNIOT_LOG_WARN("getting owner id before adding device to kit");
  }
  return {};
}

void MQTTDevice::unsubscribeFromAll() {
  while (!mTopics.isEmpty()) {
    auto topic = mTopics.hardPop();
    if (mpKit) {
      mpKit->client()->unsubscribe(topic.c_str());
    }
  }
}

bool MQTTDevice::unsubscribe(const String &topic) {
  mTopics.removeOne(topic);
  if (mpKit) {
    auto unsubscribed = mpKit->client()->unsubscribe(topic.c_str());
    UNIOT_LOG_TRACE_IF(!unsubscribed, "failed to unsubscribe from topic: %s", topic.c_str());
    return unsubscribed;
  }
  return true;
}

const String &MQTTDevice::subscribe(const String &topic) {
  mTopics.pushUnique(topic);
  if (mpKit) {
    auto subscribed = mpKit->client()->subscribe(topic.c_str());
    UNIOT_LOG_TRACE_IF(!subscribed, "failed to subscribe to topic: %s", topic.c_str());
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

void MQTTDevice::publish(const String &topic, const Bytes &payload, bool retained, bool sign) {
  if (mpKit) {
    auto msg = mpKit->_buildCOSEMessage(payload, sign);
    mpKit->client()->publish(topic.c_str(), msg.raw(), msg.size(), retained);
  }
}

void MQTTDevice::publishDevice(const String &subTopic, const Bytes &payload, bool retained, bool sign) {
  if (mpKit) {
    publish(mpKit->getPath().buildDevicePath(subTopic), payload, retained, sign);
  }
}

void MQTTDevice::publishGroup(const String &groupId, const String &subTopic, const Bytes &payload, bool retained, bool sign) {
  if (mpKit) {
    publish(mpKit->getPath().buildGroupPath(groupId, subTopic), payload, retained, sign);
  }
}

void MQTTDevice::publishEmptyDevice(const String &subTopic) {
  if (mpKit) {
    mpKit->client()->publish(mpKit->getPath().buildDevicePath(subTopic).c_str(), nullptr, 0, true);
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