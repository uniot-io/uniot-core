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
#include <ISchedulerConnectionKit.h>
#include <TaskScheduler.h>
#include <coredecls.h>
#include <time.h>

uint32_t sntp_update_delay_MS_rfc_not_less_than_15000() {
  return 10 * 60 * 1000UL;  // 10 minutes
}

namespace uniot {
class Date : public ISchedulerConnectionKit, public CBORStorage {
 public:
  Date() : CBORStorage("date.cbor") {
    settimeofday_cb([](bool from_sntp) {
      UNIOT_LOG_INFO("Time is set from %s", from_sntp ? "SNTP" : "RTC");
    });
    configTime(0, 0, "pool.ntp.org", "time.google.com", "time.nist.gov");

    this->restore();
    _initTasks();
  }

  time_t now() {
    return time(nullptr);
  }

  String getFormattedTime() {
    tm tm;
    auto currentEpoc = this->now();
    localtime_r(&currentEpoc, &tm);

    auto hours = tm.tm_hour < 10 ? "0" + String(tm.tm_hour) : String(tm.tm_hour);
    auto minutes = tm.tm_min < 10 ? "0" + String(tm.tm_min) : String(tm.tm_min);
    auto seconds = tm.tm_sec < 10 ? "0" + String(tm.tm_sec) : String(tm.tm_sec);

    return String(tm.tm_year + 1900) + "-" + String(tm.tm_mon + 1) + "-" + String(tm.tm_mday) + " " +
           hours + ":" + minutes + ":" + seconds;
  }

  void pushTo(TaskScheduler *scheduler) override {
    scheduler->push(mTaskStore);
  }

  void attach() override {
    mTaskStore->attach(5 * 60 * 1000UL);  // 5 minutes
  }

  bool store() override {
    CBORStorage::object().put("epoch", (long)this->now());
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

 private:
  void _initTasks() {
    mTaskStore = TaskScheduler::make([&](short t) {
      if (!this->store()) {
        UNIOT_LOG_ERROR("failed to store current epoch in CBORStorage");
      }
    });
  }

  TaskScheduler::TaskPtr mTaskStore;
};
}  // namespace uniot
