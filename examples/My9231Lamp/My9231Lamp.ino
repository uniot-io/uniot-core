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
    Lamp.setRed(msg ? 100 : 0);
    Lamp.update();
  }
});

namespace uniot {
namespace primitive {
Object lamp_update(Root root, VarObject env, VarObject list) {
  exportPrimitiveNameTo(name);
  PrimitiveExpeditor expiditor(name, root, env, list);
  expiditor.assertArgs(5, Lisp::Int, Lisp::Int, Lisp::Int, Lisp::BoolInt, Lisp::BoolInt);
  auto red = expiditor.getArgInt(0);
  auto green = expiditor.getArgInt(1);
  auto blue = expiditor.getArgInt(2);
  auto warm = expiditor.getArgInt(3);
  auto cool = expiditor.getArgInt(4);

  red = std::min(255, std::max(0, red));
  green = std::min(255, std::max(0, green));
  blue = std::min(255, std::max(0, blue));
  warm = warm > 0 ? 255 : 0;
  cool = cool > 0 ? 255 : 0;

  Lamp.off();
  Lamp.set(red, green, blue, warm, cool);
  Lamp.update();

  return expiditor.makeBool(false);
}
}  // namespace primitive
}  // namespace uniot

auto taskPrintHeap = TaskScheduler::make([](short t) {
  Serial.println(ESP.getFreeHeap());
});

auto taskPrintTime = TaskScheduler::make([](short t) {
  Serial.println(Date::getFormattedTime());
});

void inject() {
  Lamp.off();
  auto &MainAppKit = AppKit::getInstance(0, LOW, 2);

  MainAppKit.getLisp().pushPrimitive(globalPrimitive(lamp_update));

  MainEventBus.registerKit(&MainAppKit);
  MainEventBus.registerEntity(NetworkLedListener.listenToEvent(NetworkDevice::Topic::NETWORK_LED));

  MainScheduler.push(&MainAppKit)
      ->push(taskPrintTime)
      ->push(taskPrintHeap);

  taskPrintHeap->attach(500);
  taskPrintTime->attach(500);

  MainAppKit.begin();

  UNIOT_LOG_INFO("%s: %s", "CHIP_ID", String(ESP.getChipId(), HEX).c_str());
  UNIOT_LOG_INFO("%s: %s", "DEVICE_ID", MainAppKit.getCredentials().getDeviceId().c_str());
  UNIOT_LOG_INFO("%s: %s", "OWNER_ID", MainAppKit.getCredentials().getOwnerId().c_str());
}
