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

/** @cond */
/**
 * DO NOT DELETE THE "date-time" GROUP DEFINITION BELOW.
 * Used to create the Uniot Lisp topic in the documentation. If you want to delete this file,
 * please paste the group definition into another file and delete this one.
 */
/** @endcond */

/**
 * @defgroup date-time Date and Time Management
 * @ingroup utils
 * @brief A class for managing date and time in Uniot devices
 */

#pragma once

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <coredecls.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <esp_sntp.h>
#endif

#include <DateEvents.h>
#include <CBORStorage.h>
#include <EventEmitter.h>
#include <IExecutor.h>
#include <SimpleNTP.h>
#include <Singleton.h>
#include <time.h>

namespace uniot {
/**
 * @brief Time and date management class for Uniot devices
 * @defgroup date-time-main Main Component
 * @ingroup date-time
 * @{
 *
 * The Date class provides functionality for managing time synchronization via NTP
 * and persisting the system time between device reboots. It implements:
 * - IExecutor: For scheduled time persistence tasks
 * - CBORStorage: For time persistence to flash storage
 * - Singleton: To ensure only one instance manages time
 * - CoreEventEmitter: To emit events on time sync
 */
class Date : public IExecutor, public CBORStorage, public Singleton<Date>, public CoreEventEmitter {
  friend class Singleton<Date>;

 public:
  /**
   * @brief Returns the current Unix timestamp
   *
   * @retval time_t Current time as Unix timestamp (seconds since epoch)
   */
  static time_t now() {
    return getInstance()._now();
  }

  /**
   * @brief Gets the current time in human-readable format
   *
   * @retval String Time in "YYYY-MM-DD HH:MM:SS" format
   */
  static String getFormattedTime() {
    return getInstance()._getFormattedTime();
  }

  /**
   * @brief Stores the current time to persistent storage
   *
   * Saves the current epoch time to CBOR storage to preserve
   * time information between device reboots.
   *
   * @retval true Storage operation was successful
   * @retval false Storage operation failed
   */
  bool store() override {
    CBORStorage::object().put("epoch", static_cast<int64_t>(this->_now()));
    return CBORStorage::store();
  }

  /**
   * @brief Restores time from persistent storage
   *
   * Loads the last saved epoch time from CBOR storage and
   * sets the system time accordingly.
   *
   * @retval true Restore operation was successful
   * @retval false Restore operation failed
   */
  bool restore() override {
    if (CBORStorage::restore()) {
      auto currentEpoch = CBORStorage::object().getInt("epoch");
      _setTime(currentEpoch);
      return true;
    }
    UNIOT_LOG_ERROR("%s", "epoch not restored");
    return false;
  }

  /**
   * @brief Periodic execution callback
   *
   * Called periodically to store the current time to persistent storage.
   *
   * @param _ Unused parameter required by IExecutor interface
   */
  virtual void execute(short _) override {
    if (!this->store()) {
      UNIOT_LOG_ERROR("failed to store current epoch in CBORStorage");
    }
  }

  /**
   * @brief Forces immediate NTP time synchronization
   *
   * Reconfigures the NTP client and initiates an immediate
   * time sync request regardless of the regular sync schedule.
   */
  void forceSync() {
    _reconfigure();
    auto epoch = mSNTP.getNtpTime();
    if (epoch) {
      _setTime(epoch);
    }
  }

 private:
  /**
   * @brief Private constructor for singleton pattern
   *
   * Sets up time synchronization callbacks and initializes the system.
   * Configures platform-specific time sync notification handlers.
   */
  Date() : CBORStorage("date.cbor") {
#if defined(ESP8266)
    settimeofday_cb([this](bool from_sntp) {
      Date::getInstance()._timeSyncCallback();
      UNIOT_LOG_INFO("Time is set from %s", from_sntp ? "SNTP" : "RTC");
    });
#elif defined(ESP32)
    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    sntp_set_time_sync_notification_cb([](struct timeval *tv) {
      Date::getInstance()._timeSyncCallback();
      UNIOT_LOG_INFO("Time is set from SNTP");
    });
#endif
    mSNTP.setSyncTimeCallback([](time_t epoch) {
      Date::getInstance()._timeSyncCallback();
      UNIOT_LOG_INFO("Time is forced to synchronize with SNTP");
    });

    _reconfigure();

    this->restore();
  }

  /**
   * @brief Callback triggered when time synchronization occurs
   *
   * Stores the newly synchronized time and emits an event
   * to notify system components of the time change.
   */
  void _timeSyncCallback() {
    execute(0);
    CoreEventEmitter::emitEvent(uniot::events::date::Topic::TIME, uniot::events::date::Msg::SYNCED);
  }

  /**
   * @brief Sets the system time to the specified epoch
   *
   * Platform-specific implementation to set the device's system time.
   *
   * @param epoch Unix timestamp to set as current time
   * @retval true Time was set successfully
   * @retval false Failed to set time
   */
  bool _setTime(time_t epoch) {
#if defined(ESP8266)
    tune_timeshift64(epoch * 1000000ULL);
#elif defined(ESP32)
    timeval tv = {epoch, 0};
    if (settimeofday(&tv, nullptr) != 0) {
      UNIOT_LOG_ERROR("Failed to set system time");
      return false;
    }
#endif
    return true;
  }

  /**
   * @brief Reconfigures the NTP client with server settings
   *
   * Sets up the time zone (UTC+0) and NTP server pool.
   */
  void _reconfigure() {
    configTime(0, 0, SimpleNTP::servers[0], SimpleNTP::servers[1], SimpleNTP::servers[2]);
  }

  /**
   * @brief Returns the current system time
   *
   * @retval time_t Current Unix timestamp
   */
  time_t _now() {
    return time(nullptr);
  }

  /**
   * @brief Formats the current time as a human-readable string
   *
   * Converts the current Unix timestamp to a formatted date and time string
   * in the format "YYYY-MM-DD HH:MM:SS".
   *
   * @retval String Formatted date and time
   */
  String _getFormattedTime() {
    tm tm;
    auto currentEpoc = this->_now();
    localtime_r(&currentEpoc, &tm);

    char formattedTime[72] = {0};
    snprintf(formattedTime, sizeof(formattedTime), "%04d-%02d-%02d %02d:%02d:%02d",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec);

    return String(formattedTime);
  }

  /** @brief NTP client instance */
  SimpleNTP mSNTP;
};
/** @} */
}  // namespace uniot
