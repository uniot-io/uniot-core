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

#include <functional>
#include <ClearQueue.h>
#include <Bytes.h>

namespace uniot
{
class MQTTKit;

class MQTTDevice
{
  friend class MQTTKit;

protected:
  using Handler = std::function<void(MQTTDevice *device, const String &topic, const Bytes &payload)>;

  ClearQueue<String> *topics()
  {
    return &mTopics;
  }

  void kit(MQTTKit *kit)
  {
    mpKit = kit;
  }

  Handler handler()
  {
    return mHandler;
  }

  ClearQueue<String> mTopics;
  MQTTKit *mpKit;
  Handler mHandler;

public:
  MQTTDevice(Handler handler = nullptr) : mpKit(nullptr), mHandler(handler) {}
  virtual ~MQTTDevice();
  void subscribe(const String &topic);
  void publish(const String &topic, const Bytes &payload);
  bool isSubscribed(const String &topic);
};
} // namespace uniot