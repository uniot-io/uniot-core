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

#if defined(ESP8266)

#include <stddef.h>
#include <stdint.h>

extern "C" {
#include "c_types.h"
#include "eagle_soc.h"
#include "ets_sys.h"
#include "osapi.h"
}

namespace uniot {
extern "C" {
typedef struct _ETSTIMER_ ETSTimer;
}

/**
 * @defgroup scheduler_task_esp8266 ESP8266 Task
 * @ingroup scheduler_task
 * @brief ESP8266-specific task implementation for the Uniot Core.
 *
 * This class provides platform-specific task scheduling functionality
 * using the ESP8266's native timer system. It wraps the ESP8266 timer API
 * to provide a consistent interface for the scheduler.
 * @{
 */
class ESP8266Task {
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
  ESP8266Task()
      : mpTimer(nullptr) {}

  /**
   * @brief Destructor that ensures the timer is detached
   */
  virtual ~ESP8266Task() {
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
    attach_arg(ms, repeat, reinterpret_cast<TaskArgCallback>(callback), 0);
  }

  /**
   * @brief Attach a typed callback with argument to run periodically
   *
   * @param ms Millisecond interval between executions
   * @param repeat True to run repeatedly, false to run once
   * @param callback Function to execute
   * @param arg Argument to pass to the callback (must fit in 4 bytes)
   */
  template <typename T>
  void attach(uint32_t ms, bool repeat, TaskTypeCallback<volatile T> callback, volatile T arg) {
    static_assert(sizeof(volatile T) <= sizeof(uint32_t), "sizeof arg must be <= sizeof(uint32_t), i.e 4 bytes");
    attach_arg(ms, repeat, reinterpret_cast<TaskArgCallback>(callback), reinterpret_cast<volatile void *>(arg));
  }

  /**
   * @brief Stop and detach the timer
   */
  void detach() {
    if (mpTimer) {
      os_timer_disarm(mpTimer);
      delete mpTimer;
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
  ETSTimer *mpTimer; ///< Pointer to the ESP8266 timer

  /**
   * @brief Internal implementation for attaching callbacks
   *
   * @param ms Millisecond interval
   * @param repeat True to repeat, false for one-time execution
   * @param callback Function to execute
   * @param arg Argument to pass to the callback
   */
  void attach_arg(uint32_t ms, bool repeat, TaskArgCallback callback, volatile void *arg) {
    if (mpTimer) {
      os_timer_disarm(mpTimer);
    } else {
      mpTimer = new ETSTimer;
    }

    os_timer_setfn(mpTimer, reinterpret_cast<ETSTimerFunc *>(callback), const_cast<void*>(arg));
    os_timer_arm(mpTimer, ms, repeat);
  }
};
/** @} */
}  // namespace uniot

#endif  // defined(ESP8266)

