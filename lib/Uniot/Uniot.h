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

#include <Arduino.h>
#include <Credentials.h>
#include <Date.h>
#include <EventBus.h>
#include <Logger.h>
#include <TaskScheduler.h>

class UniotCore {
 public:
  UniotCore() : mScheduler(), mEventBus(FOURCC(main)) {}

  void begin(uint32_t eventBusTaskPeriod = 10, uint32_t storeDateTaskPeriod = 5 * 60 * 1000UL) {
    UNIOT_LOG_SET_READY();

    auto taskHandleEventBus = uniot::TaskScheduler::make(mEventBus);
    mScheduler.push("event_bus", taskHandleEventBus);
    taskHandleEventBus->attach(eventBusTaskPeriod);

    if (storeDateTaskPeriod > 0) {
      auto taskStoreDate = uniot::TaskScheduler::make(uniot::Date::getInstance());
      mScheduler.push("store_date", taskStoreDate);
      taskStoreDate->attach(storeDateTaskPeriod);
    }
  }

  void loop() {
    mScheduler.loop();
  }

  uniot::CoreEventBus& getEventBus() {
    return mEventBus;
  }

  uniot::TaskScheduler& getScheduler() {
    return mScheduler;
  }

 private:
  uniot::TaskScheduler mScheduler;
  uniot::CoreEventBus mEventBus;
};

UniotCore Uniot;