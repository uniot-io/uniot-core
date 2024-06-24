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
#include <PZEM004Tv30.h>
#include <SoftwareSerial.h>
#include <Logger.h>

#define RX_PIN 4 // GPIO4
#define TX_PIN 5 // GPIO5


SoftwareSerial pzemSWSerial(RX_PIN, TX_PIN);

class EnergyMonitor {
 public:
  EnergyMonitor() {
    _setup();
  }

  uint16_t getPower() {
    auto power = pzem.power();
    if (isnan(power)) {
      UNIOT_LOG_PRINT("EnergyMonitor: Power reading failed");
      return 0;
    }
    return (uint16_t)power;
  }

  uint16_t getVoltage() {
    auto voltage = pzem.voltage();
    if (isnan(voltage)) {
      UNIOT_LOG_PRINT("EnergyMonitor: Voltage reading failed");
      return 0;
    }
    return (uint16_t)voltage;
  }

 private:
  void _setup() {
    // GPIO setup.
    pinMode(RX_PIN, INPUT);
    pinMode(TX_PIN, OUTPUT);

    pzem = PZEM004Tv30(pzemSWSerial);
  }

  PZEM004Tv30 pzem;
};
