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
#include "CSE7766.h"
#include <SoftwareSerial.h>
#include <Logger.h>

#define RELAY_PIN 13

class Tomzn1PRelay {
 public:
  Tomzn1PRelay() {
    _setup();
  }

  auto getPower() {
    auto power = cse7766.getActivePower();
    return (uint16_t)power;
  }

  auto getVoltage() {
    auto voltage = cse7766.getVoltage();
    return (uint16_t)voltage;
  }

  void setSupply(bool status) {
    digitalWrite(RELAY_PIN, status ? HIGH : LOW);
  }

 private:
  void _setup() {
    cse7766.setRX(1);
    cse7766.begin();

    // GPIO setup.
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, HIGH);
  }

  CSE7766 cse7766;
};
