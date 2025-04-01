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
/**
 * @brief Device class for system monitoring and diagnostics
 * @defgroup app-kit-top-device Table of Processes
 * @ingroup app-kit
 * @{
 *
 * TopDevice provides system monitoring functionality similar to the Unix 'top' command,
 * exposing task statistics, memory usage, and system information through MQTT.
 * This allows for remote monitoring of device performance and resource utilization.
 */
class TopDevice : public MQTTDevice {
 public:
  /**
   * @brief Construct a new TopDevice instance
   *
   * Initializes the TopDevice with default values and no scheduler attached.
   */
  TopDevice()
      : MQTTDevice(),
        mpScheduler(nullptr) {}

  /**
   * @brief Set up MQTT topic subscriptions for device monitoring
   *
   * Subscribes to debug topics that trigger system information requests:
   * - debug/top/ask: Request task and performance statistics
   * - debug/mem/ask: Request memory usage information
   *
   * @override Implements MQTTDevice::syncSubscriptions
   */
  virtual void syncSubscriptions() override {
    mTopicTopAsk = MQTTDevice::subscribeDevice("debug/top/ask");
    mTopicMemAsk = MQTTDevice::subscribeDevice("debug/mem/ask");
  }

  /**
   * @brief Associate a TaskScheduler with this monitoring device
   *
   * Links a TaskScheduler to enable task performance monitoring.
   * The scheduler is required for the 'top' functionality to work.
   *
   * @param scheduler Reference to the TaskScheduler to monitor
   */
  void setScheduler(const TaskScheduler& scheduler) {
    mpScheduler = &scheduler;
  }

  /**
   * @brief Handle incoming MQTT messages
   *
   * Routes incoming messages to the appropriate handler based on the topic.
   *
   * @param topic The MQTT topic of the received message
   * @param payload The binary payload of the message
   * @override Implements MQTTDevice::handle
   */
  virtual void handle(const String& topic, const Bytes& payload) override {
    if (MQTTDevice::isTopicMatch(mTopicTopAsk, topic)) {
      handleTop();
      return;
    }
    if (MQTTDevice::isTopicMatch(mTopicMemAsk, topic)) {
      handleMem();
      return;
    }
  }

  /**
   * @brief Handle request for task and system performance data
   *
   * Collects and publishes information about:
   * - All registered tasks (name, status, execution time)
   * - Idle time of the system
   * - Current timestamp and system uptime
   *
   * Data is sent via MQTT in CBOR format to the "debug/top" topic.
   */
  void handleTop() {
    if (mpScheduler) {
      CBORObject packet;
      auto tasksObj = packet.putMap("tasks");
      uint64_t tasksElapsedMs = 0;
      mpScheduler->exportTasksInfo([&](const char* name, bool isAttached, uint64_t elapsedMs) {
        tasksElapsedMs += elapsedMs;
        tasksObj.putArray(name)
            .append(isAttached)  // Whether the task is currently attached to the scheduler
            .append(elapsedMs);  // Total execution time in milliseconds
      });
      auto idleMs = mpScheduler->getTotalElapsedMs() - tasksElapsedMs;
      packet.put("idle", idleMs);
      packet.put("timestamp", static_cast<int64_t>(Date::now()));
      packet.put("uptime", static_cast<uint64_t>(millis()));

      MQTTDevice::publishDevice("debug/top", packet.build());
    }
  }

  /**
   * @brief Handle request for memory usage information
   *
   * Collects and publishes information about available free heap memory.
   * Data is sent via MQTT in CBOR format to the "debug/mem" topic.
   */
  void handleMem() {
    CBORObject packet;
    packet.put("available", static_cast<uint64_t>(ESP.getFreeHeap()));
    MQTTDevice::publishDevice("debug/mem", packet.build());
  }

 private:
  const TaskScheduler* mpScheduler;  ///< Pointer to the monitored task scheduler
  String mTopicTopAsk;               ///< MQTT topic for task performance requests
  String mTopicMemAsk;               ///< MQTT topic for memory usage requests
};
/** @} */
}  // namespace uniot
