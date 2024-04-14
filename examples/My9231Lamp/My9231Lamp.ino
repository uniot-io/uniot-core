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

#include <AppKit.h>
#include <Date.h>
#include <LispPrimitives.h>
#include <Logger.h>
#include <Uniot.h>

#include "My9231Lamp.h"

using namespace uniot;

My9231Lamp Lamp;

CoreCallbackEventListener NetworkLedListener([](int topic, int msg) {
  if (topic == NetworkDevice::Topic::NETWORK_LED) {
    Lamp.off();
    Lamp.setRed(msg ? 10 : 0);
    Lamp.update();
  }
});

Object lamp_update(Root root, VarObject env, VarObject list) {
  auto expeditor = PrimitiveExpeditor::describe(getPrimitiveName(), Lisp::Bool, 5, Lisp::Int, Lisp::Int, Lisp::Int, Lisp::BoolInt, Lisp::BoolInt)
                       .init(root, env, list);
  expeditor.assertDescribedArgs();
  auto red = expeditor.getArgInt(0);
  auto green = expeditor.getArgInt(1);
  auto blue = expeditor.getArgInt(2);
  auto warm = expeditor.getArgInt(3);
  auto cool = expeditor.getArgInt(4);

  red = std::min(255, std::max(0, red));
  green = std::min(255, std::max(0, green));
  blue = std::min(255, std::max(0, blue));

  warm = std::min(255, std::max(0, warm));
  cool = std::min(255, std::max(0, cool));
  // warm = warm > 0 ? 255 : 0; // Sonoff B1R2
  // cool = cool > 0 ? 255 : 0; // Sonoff B1R2

  Lamp.off();
  Lamp.set(red, green, blue, warm, cool);
  Lamp.update();

  return expeditor.makeBool(false);
}

auto taskPrintHeap = TaskScheduler::make([](short t) {
  Serial.println(ESP.getFreeHeap());
});

auto taskPrintTime = TaskScheduler::make([](short t) {
  Serial.println(Date::getFormattedTime());
});

void inject() {
  Lamp.off();
  auto &MainAppKit = AppKit::getInstance(0, LOW, 2);

  MainAppKit.getLisp().pushPrimitive(lamp_update);

  MainEventBus.registerKit(MainAppKit);
  MainEventBus.registerEntity(NetworkLedListener.listenToEvent(NetworkDevice::Topic::NETWORK_LED));

  MainScheduler
      .push(MainAppKit)
      .push("print_time", taskPrintTime)
      .push("print_heap", taskPrintHeap);

  taskPrintHeap->attach(500);
  taskPrintTime->attach(500);

  MainAppKit.attach();

  UNIOT_LOG_INFO("%s: %s", "CHIP_ID", String(ESP.getChipId(), HEX).c_str());
  UNIOT_LOG_INFO("%s: %s", "DEVICE_ID", MainAppKit.getCredentials().getDeviceId().c_str());
  UNIOT_LOG_INFO("%s: %s", "OWNER_ID", MainAppKit.getCredentials().getOwnerId().c_str());
}
