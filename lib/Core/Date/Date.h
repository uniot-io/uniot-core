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

#include <CBORStorage.h>
#include <ESP8266WiFi.h>
#include <TaskScheduler.h>
#include <coredecls.h>
#include <time.h>

namespace uniot {
class Date : public IExecutor, public CBORStorage {
 public:
  Date(Date const &) = delete;
  void operator=(Date const &) = delete;

  static Date &getInstance() {
    static Date instance;
    return instance;
  }

  static time_t now() {
    return getInstance()._now();
  }

  static String getFormattedTime() {
    return getInstance()._getFormattedTime();
  }

  bool store() override {
    CBORStorage::object().put("epoch", (long)this->_now());
    return CBORStorage::store();
  }

  bool restore() override {
    if (CBORStorage::restore()) {
      auto currentEpoc = CBORStorage::object().getInt("epoch");
      tune_timeshift64(currentEpoc * 1000000ULL);
      return true;
    }
    UNIOT_LOG_ERROR("%s", "epoch not restored");
    return false;
  }

  virtual uint8_t execute() override {
    if (!this->store()) {
      UNIOT_LOG_ERROR("failed to store current epoch in CBORStorage");
    }
    return 0;
  }

 private:
  Date() : CBORStorage("date.cbor") {
    settimeofday_cb([this](bool from_sntp) {
      UNIOT_LOG_INFO("Time is set from %s", from_sntp ? "SNTP" : "RTC");
    });
    configTime(0, 0, "pool.ntp.org", "time.google.com", "time.nist.gov");

    this->restore();
  }

  time_t _now() {
    return time(nullptr);
  }

  String _getFormattedTime() {
    tm tm;
    auto currentEpoc = this->_now();
    localtime_r(&currentEpoc, &tm);

    auto hours = tm.tm_hour < 10 ? "0" + String(tm.tm_hour) : String(tm.tm_hour);
    auto minutes = tm.tm_min < 10 ? "0" + String(tm.tm_min) : String(tm.tm_min);
    auto seconds = tm.tm_sec < 10 ? "0" + String(tm.tm_sec) : String(tm.tm_sec);

    return String(tm.tm_year + 1900) + "-" + String(tm.tm_mon + 1) + "-" + String(tm.tm_mday) + " " +
           hours + ":" + minutes + ":" + seconds;
  }
};
}  // namespace uniot
