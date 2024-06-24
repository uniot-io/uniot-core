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

#include "Tomzn1PRelay.h"
#include <iostream>

using namespace uniot;
using namespace std;

Tomzn1PRelay Relay;

Object power_read(Root root, VarObject env, VarObject list) {
  auto expeditor = PrimitiveExpeditor::describe(getPrimitiveName(), Lisp::Int, 0)
                        .init(root, env, list);
  auto power = Relay.getPower();
  return expeditor.makeInt(power);
}
Object voltage_read(Root root, VarObject env, VarObject list) {
  auto expeditor = PrimitiveExpeditor::describe(getPrimitiveName(), Lisp::Int, 0)
                        .init(root, env, list);
  auto voltage = Relay.getVoltage();
  return expeditor.makeInt(voltage);
}
Object supply_set(Root root, VarObject env, VarObject list) {
  auto expeditor = PrimitiveExpeditor::describe(getPrimitiveName(), Lisp::Bool, 1, Lisp::Bool)
                        .init(root, env, list);
  expeditor.assertDescribedArgs();
  auto status = expeditor.getArgBool(0);
  Relay.setSupply(status);
  return expeditor.makeBool(true);
}

auto taskPrintHeap = TaskScheduler::make([](short t) {
  Serial.println(ESP.getFreeHeap());
});

auto taskPrintTime = TaskScheduler::make([](short t) {
  Serial.println(Date::getFormattedTime());
});

void inject() {
  auto &MainAppKit = AppKit::getInstance(0, LOW, 2);

  MainAppKit.getLisp().pushPrimitive(power_read);
  MainAppKit.getLisp().pushPrimitive(voltage_read);
  MainAppKit.getLisp().pushPrimitive(supply_set);

  MainEventBus.registerKit(&MainAppKit);

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
