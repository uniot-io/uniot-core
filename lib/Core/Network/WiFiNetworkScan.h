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

/**
 * @file WiFiNetworkScan.h
 * @brief Platform-specific WiFi network scanning utilities
 * @defgroup wifi_scan WiFi Network Scanning
 * @ingroup network
 * @{
 *
 * This file provides platform-specific implementations for WiFi network scanning
 * on ESP8266 and ESP32 microcontrollers. It abstracts the differences between
 * the two platforms while providing a consistent interface for asynchronous
 * network discovery.
 *
 * The implementations handle:
 * - Asynchronous WiFi network scanning
 * - Platform-specific encryption type detection
 * - Task-based completion handling for ESP32
 * - Callback-based completion for ESP8266
 *
 * Example usage:
 * @code
 * #if defined(ESP32)
 * ESP32WifiScan scanner;
 * scheduler.push(scanner.getTask());
 * #elif defined(ESP8266)
 * ESP8266WifiScan scanner;
 * #endif
 *
 * scanner.scanNetworksAsync([](int networkCount) {
 *   for (int i = 0; i < networkCount; i++) {
 *     Serial.println(WiFi.SSID(i));
 *   }
 * });
 * @endcode
 */

namespace uniot {
#if defined(ESP8266)
/**
 * @brief WiFi network scanner implementation for ESP8266
 *
 * This class provides WiFi network scanning functionality specifically for
 * ESP8266 microcontrollers. It wraps the native ESP8266WiFi library to
 * provide asynchronous scanning capabilities with callback-based completion.
 */
class ESP8266WifiScan {
 public:
  /**
   * @brief Check if a network uses encryption
   * @param encryptionType The encryption type returned by WiFi scan
   * @retval int 1 if network is secured, 0 if open
   */
  static int isSecured(int encryptionType) {
    return static_cast<int>(encryptionType != ENC_TYPE_NONE);
  }

  /**
   * @brief Start asynchronous WiFi network scan
   * @param onComplete Callback function called when scan completes
   * @param showHidden Whether to include hidden networks in scan results
   */
  void scanNetworksAsync(std::function<void(int)> onComplete, bool showHidden = false) {
    WiFi.scanNetworksAsync(onComplete, showHidden);
  }
};
#endif

#if defined(ESP32)
/**
 * @brief WiFi network scanner implementation for ESP32
 *
 * This class provides WiFi network scanning functionality specifically for
 * ESP32 microcontrollers. Since ESP32's WiFi library doesn't provide native
 * callback support for scan completion, this implementation uses a scheduled
 * task to poll for scan completion and trigger the callback.
 */
class ESP32WifiScan {
 public:
  /**
   * @brief Construct a new ESP32 WiFi scanner
   *
   * Initializes the scanner and creates a scheduled task for monitoring
   * scan completion status.
   */
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

  /**
   * @brief Check if a network uses encryption
   * @param encryptionType The encryption type returned by WiFi scan
   * @retval int 1 if network is secured, 0 if open
   */
  static int isSecured(int encryptionType) {
    return static_cast<int>(encryptionType != WIFI_AUTH_OPEN);
  }

  /**
   * @brief Get the scan completion monitoring task
   * @retval TaskScheduler::TaskPtr Pointer to the task that monitors scan completion
   * @note This task must be added to a TaskScheduler for the scanner to work properly
   */
  TaskScheduler::TaskPtr getTask() {
    return mTaskCompleteScan;
  }

  /**
   * @brief Start asynchronous WiFi network scan
   * @param onComplete Callback function called when scan completes with network count
   * @param showHidden Whether to include hidden networks in scan results
   * @note The scan completion task will be automatically attached with 500ms interval
   */
  void scanNetworksAsync(std::function<void(int)> onComplete, bool showHidden = false) {
    WiFi.scanNetworks(true, showHidden);
    mOnComplete = onComplete;
    mTaskCompleteScan->attach(500);
  }

 private:
  std::function<void(int)> mOnComplete;      ///< Callback function for scan completion
  TaskScheduler::TaskPtr mTaskCompleteScan;  ///< Task for monitoring scan completion
};
#endif
}  // namespace uniot

/** @} */