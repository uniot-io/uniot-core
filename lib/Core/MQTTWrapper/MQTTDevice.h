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

#include <ClearQueue.h>
#include <Bytes.h>

namespace uniot
{
class MQTTKit;

class MQTTDevice
{
  friend class MQTTKit;

protected:
  ClearQueue<String> *topics()
  {
    return &mTopics;
  }

  void kit(MQTTKit *kit)
  {
    mpKit = kit;
  }

  virtual void handle(const String &topic, const Bytes &payload) = 0; 

  ClearQueue<String> mTopics;
  MQTTKit *mpKit;

public:
  MQTTDevice() : mpKit(nullptr) {}
  virtual ~MQTTDevice();
  void subscribe(const String &topic);
  void subscribeDevice(const String &subTopic);
  void subscribeGroup(const String &subTopic);
  void publish(const String &topic, const Bytes &payload, bool retained = false);
  void publishDevice(const String &subTopic, const Bytes &payload, bool retained = false);
  void publishGroup(const String &subTopic, const Bytes &payload, bool retained = false);
  bool isSubscribed(const String &topic);
};
} // namespace uniot