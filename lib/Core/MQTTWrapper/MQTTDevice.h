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
#include <IterableQueue.h>

namespace uniot {
class MQTTKit;

class MQTTDevice {
  friend class MQTTKit;

 public:
  MQTTDevice() : mpKit(nullptr) {}
  virtual ~MQTTDevice();

  const String &subscribe(const String &topic);
  String subscribeDevice(const String &subTopic);
  String subscribeGroup(const String &groupId, const String &subTopic);
  bool unsubscribe(const String &topic);

  virtual void syncSubscriptions() = 0;  // NOTE: subscriptions that depend on credentials should be reconstructed here
  void unsubscribeFromAll();

  bool isSubscribed(const String &topic);
  bool isTopicMatch(const String &storedTopic, const String &incomingTopic) const;

  void publish(const String &topic, const Bytes &payload, bool retained = false, bool sign = false);
  void publishDevice(const String &subTopic, const Bytes &payload, bool retained = false, bool sign = false);
  void publishGroup(const String &groupId, const String &subTopic, const Bytes &payload, bool retained = false, bool sign = false);

 protected:
  virtual void handle(const String &topic, const Bytes &payload) = 0;

 private:
  IterableQueue<String> *topics() {
    return &mTopics;
  }

  void kit(MQTTKit *kit) {
    mpKit = kit;
  }

  IterableQueue<String> mTopics;
  MQTTKit *mpKit;
};
}  // namespace uniot
