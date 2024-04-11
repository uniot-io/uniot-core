/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2024 Uniot <contact@uniot.io>
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

#include <CBORObject.h>
#include <Date.h>
#include <MQTTDevice.h>
#include <TaskScheduler.h>

namespace uniot {

class TopDevice : public MQTTDevice {
 public:
  TopDevice()
      : MQTTDevice(),
        mpScheduler(nullptr) {}

  virtual void syncSubscriptions() override {
    mTopicTopAsk = MQTTDevice::subscribeDevice("top/ask");
  }

  void setScheduler(const TaskScheduler& scheduler) {
    mpScheduler = &scheduler;
  }

  virtual void handle(const String& topic, const Bytes& payload) override {
    if (mpScheduler && MQTTDevice::isTopicMatch(mTopicTopAsk, topic)) {
      CBORObject packet;
      auto tasksObj = packet.putMap("tasks");
      uint64_t tasksElapsedMs = 0;
      mpScheduler->exportTasksInfo([&](const char* name, bool isAttached, uint64_t elapsedMs) {
        tasksElapsedMs += elapsedMs;
        tasksObj.putArray(name)
            .append(isAttached)
            .append(elapsedMs);
      });
      auto idleMs = mpScheduler->getTotalElapsedMs() - tasksElapsedMs;
      packet.put("idle", idleMs);
      packet.put("timestamp", Date::now());

      MQTTDevice::publishDevice("top", packet.build());
    }
  }

 private:
  const TaskScheduler* mpScheduler;
  String mTopicTopAsk;
};

}  // namespace uniot