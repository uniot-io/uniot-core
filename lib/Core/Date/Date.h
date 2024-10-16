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
#pragma once

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <coredecls.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <esp_sntp.h>
#endif

#include <CBORStorage.h>
#include <EventEmitter.h>
#include <IExecutor.h>
#include <SimpleNTP.h>
#include <Singleton.h>
#include <time.h>

namespace uniot {
class Date : public IExecutor, public CBORStorage, public Singleton<Date>, public CoreEventEmitter {
  friend class Singleton<Date>;

 public:
  enum Topic { TIME = FOURCC(date) };
  enum Msg { SYNCED = 0 };

  static time_t now() {
    return getInstance()._now();
  }

  static String getFormattedTime() {
    return getInstance()._getFormattedTime();
  }

  bool store() override {
    CBORStorage::object().put("epoch", static_cast<int64_t>(this->_now()));
    return CBORStorage::store();
  }

  bool restore() override {
    if (CBORStorage::restore()) {
      auto currentEpoch = CBORStorage::object().getInt("epoch");
      _setTime(currentEpoch);
      return true;
    }
    UNIOT_LOG_ERROR("%s", "epoch not restored");
    return false;
  }

  virtual void execute(short _) override {
    if (!this->store()) {
      UNIOT_LOG_ERROR("failed to store current epoch in CBORStorage");
    }
  }

  void forceSync() {
    _reconfigure();
    auto epoch = mSNTP.getNtpTime();
    if (epoch) {
      _setTime(epoch);
    }
  }

 private:
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

  void _timeSyncCallback() {
    execute(0);
    CoreEventEmitter::emitEvent(Topic::TIME, Msg::SYNCED);
  }

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

  void _reconfigure() {
    configTime(0, 0, SimpleNTP::servers[0], SimpleNTP::servers[1], SimpleNTP::servers[2]);
  }

  time_t _now() {
    return time(nullptr);
  }

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

  SimpleNTP mSNTP;
};
}  // namespace uniot
