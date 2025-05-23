/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2025 Uniot <contact@uniot.io>
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
#include "ESP8266WiFi.h"
#elif defined(ESP32)
#include "WiFi.h"
#endif

#include <Arduino.h>
#include <TaskScheduler.h>

namespace uniot {
#if defined(ESP8266)
class ESP8266WifiScan {
 public:
  static int isSecured(int encryptionType) {
    return static_cast<int>(encryptionType != ENC_TYPE_NONE);
  }

  void scanNetworksAsync(std::function<void(int)> onComplete, bool showHidden = false) {
    WiFi.scanNetworksAsync(onComplete, showHidden);
  }
};
#endif

#if defined(ESP32)
class ESP32WifiScan {
 public:
  ESP32WifiScan() : mOnComplete(nullptr) {
    mTaskCompleteScan = TaskScheduler::make([this](SchedulerTask &self, short times) {
      if (!mOnComplete) {
        self.detach();
        return;
      }

      auto scanCount = WiFi.scanComplete();
      if (scanCount == WIFI_SCAN_RUNNING || scanCount == WIFI_SCAN_FAILED) {
        return;
      }

      mOnComplete(scanCount);
      mOnComplete = nullptr;
      self.detach();
    });
  }

  static int isSecured(int encryptionType) {
    return static_cast<int>(encryptionType != WIFI_AUTH_OPEN);
  }

  TaskScheduler::TaskPtr getTask() {
    return mTaskCompleteScan;
  }

  void scanNetworksAsync(std::function<void(int)> onComplete, bool showHidden = false) {
    WiFi.scanNetworks(true, showHidden);
    mOnComplete = onComplete;
    mTaskCompleteScan->attach(500);
  }

 private:
  std::function<void(int)> mOnComplete;
  TaskScheduler::TaskPtr mTaskCompleteScan;
};
#endif
}  // namespace uniot