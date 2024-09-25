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

#if defined(ESP32)

#include <esp_timer.h>

namespace uniot {
class Task {
 public:
  using TaskCallback = void (*)(void);
  using TaskArgCallback = void (*)(void *);
  template <typename T>
  using TaskTypeCallback = void (*)(volatile T);

  Task()
      : mpTimer(nullptr) {}

  virtual ~Task() {
    detach();
  }

  void attach(uint32_t ms, bool repeat, TaskCallback callback) {
    attach_arg(ms, repeat, reinterpret_cast<TaskArgCallback>(callback), nullptr);
  }

  template <typename T>
  void attach(uint32_t ms, bool repeat, TaskTypeCallback<volatile T> callback, volatile T arg) {
    attach_arg(ms, repeat, reinterpret_cast<TaskArgCallback>(callback), reinterpret_cast<volatile void *>(arg));
  }

  void detach() {
    if (mpTimer) {
      esp_timer_stop(mpTimer);
      esp_timer_delete(mpTimer);
      mpTimer = nullptr;
    }
  }

  bool isAttached() {
    return mpTimer != nullptr;
  }

 private:
  esp_timer_handle_t mpTimer;

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

}  // namespace uniot

#endif  // ESP32
