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

#include <CrashStorage.h>
#include <Date.h>
#include <Uniot.h>

uniot::TaskScheduler MainScheduler;
uniot::CoreEventBus MainEventBus(FOURCC(main));

void setup() {
  UNIOT_LOG_SET_READY();

  auto taskHandleEventBus = uniot::TaskScheduler::make(MainEventBus);
  MainScheduler.push("event_bus", taskHandleEventBus);
  taskHandleEventBus->attach(100);

  auto taskStoreDate = uniot::TaskScheduler::make(uniot::Date::getInstance());
  MainScheduler.push("store_date", taskStoreDate);
  taskStoreDate->attach(5 * 60 * 1000UL);  // 5 minutes

  inject();
}

void loop() {
  MainScheduler.loop();
}

extern "C" void custom_crash_callback(struct rst_info *resetInfo, uint32_t stackStart, uint32_t stackEnd) {
  uniot::uniotCrashCallback(resetInfo, stackStart, stackEnd);
}
