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

/**
 * @file ESP32Task.h
 * @brief ESP32-specific task implementation for the Uniot Core
 */

#pragma once

#if defined(ESP32)

#include <esp_timer.h>

namespace uniot {
/**
 * @defgroup scheduler_task_esp32 ESP32 Task
 * @ingroup scheduler_task
 * @brief ESP32-specific task implementation for the Uniot Core
 *
 * This class provides platform-specific task scheduling functionality
 * using the ESP32's native timer system. It wraps the ESP32 timer API
 * to provide a consistent interface for the scheduler.
 * @{
 */
class ESP32Task {
 public:
  /**
   * @brief Callback function with no arguments
   */
  using TaskCallback = void (*)(void);

  /**
   * @brief Callback function with void pointer argument
   */
  using TaskArgCallback = void (*)(void *);

  /**
   * @brief Templated callback function with typed argument
   */
  template <typename T>
  using TaskTypeCallback = void (*)(volatile T);

  /**
   * @brief Constructor
   */
  ESP32Task()
      : mpTimer(nullptr) {}

  /**
   * @brief Destructor that ensures the timer is detached
   */
  virtual ~ESP32Task() {
    detach();
  }

  /**
   * @brief Attach a simple callback to run periodically
   *
   * @param ms Millisecond interval between executions
   * @param repeat True to run repeatedly, false to run once
   * @param callback Function to execute
   */
  void attach(uint32_t ms, bool repeat, TaskCallback callback) {
    attach_arg(ms, repeat, reinterpret_cast<TaskArgCallback>(callback), nullptr);
  }

  /**
   * @brief Attach a typed callback with argument to run periodically
   *
   * @param ms Millisecond interval between executions
   * @param repeat True to run repeatedly, false to run once
   * @param callback Function to execute
   * @param arg Argument to pass to the callback
   */
  template <typename T>
  void attach(uint32_t ms, bool repeat, TaskTypeCallback<volatile T> callback, volatile T arg) {
    attach_arg(ms, repeat, reinterpret_cast<TaskArgCallback>(callback), reinterpret_cast<volatile void *>(arg));
  }

  /**
   * @brief Stop and detach the timer
   */
  void detach() {
    if (mpTimer) {
      esp_timer_stop(mpTimer);
      esp_timer_delete(mpTimer);
      mpTimer = nullptr;
    }
  }

  /**
   * @brief Check if the timer is attached
   *
   * @retval true The timer is attached and running
   * @retval false The timer is not attached
   */
  bool isAttached() {
    return mpTimer != nullptr;
  }

 private:
  esp_timer_handle_t mpTimer; ///< Handle to the ESP32 timer

  /**
   * @brief Internal implementation for attaching callbacks
   *
   * @param ms Millisecond interval
   * @param repeat True to repeat, false for one-time execution
   * @param callback Function to execute
   * @param arg Argument to pass to the callback
   */
  void attach_arg(uint32_t ms, bool repeat, TaskArgCallback callback, volatile void *arg) {
    esp_timer_create_args_t timerConfig;
    timerConfig.callback = reinterpret_cast<esp_timer_cb_t>(callback);
    timerConfig.arg = const_cast<void*>(arg);
    timerConfig.dispatch_method = ESP_TIMER_TASK;
    timerConfig.name = "Task";

    if (mpTimer) {
      esp_timer_stop(mpTimer);
      esp_timer_delete(mpTimer);
    }

    esp_timer_create(&timerConfig, &mpTimer);

    if (repeat) {
      esp_timer_start_periodic(mpTimer, ms * 1000);
    } else {
      esp_timer_start_once(mpTimer, ms * 1000);
    }
  }
};
/** @} */

}  // namespace uniot

#endif  // ESP32
