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

#include <Uniot.h>
#include <Board-WittyCloud.h>
#include <CBORObject.h>
#include <AppKit.h>
#include <Storage.h>
#include <LispPrimitives.h>
#include <Logger.h>

using namespace uniot;

AppKit MainAppKit(MyCredentials, PIN_BUTTON, BTN_PIN_LEVEL, RED);

auto taskPrintHeap = TaskScheduler::make([&](short t) {
  Serial.println(ESP.getFreeHeap());
});

void inject()
{
  UniotPinMap.setDigitalOutput(3, RED, GREEN, BLUE);
  UniotPinMap.setDigitalInput(3, RED, GREEN, BLUE);
  UniotPinMap.setAnalogOutput(3, RED, GREEN, BLUE);
  UniotPinMap.setAnalogInput(1, LDR);

  MainBroker.connect(&MainAppKit);
  MainScheduler.push(&MainAppKit)
      ->push(taskPrintHeap);

  taskPrintHeap->attach(500);

  MainAppKit.attach();
  MainAppKit.begin();

  UNIOT_LOG_INFO("%s: %s", "CHIP_ID", String(ESP.getChipId(), HEX).c_str());
  UNIOT_LOG_INFO("%s: %s", "DEVICE_ID", MyCredentials.getDeviceId().c_str());
  UNIOT_LOG_INFO("%s: %s", "OWNER_ID", MyCredentials.getOwnerId().c_str());
}
