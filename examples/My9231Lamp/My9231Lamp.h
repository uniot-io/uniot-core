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

#include <Arduino.h>

#define DELAY_US 25  // > 12us

#define DI_PIN 13   // 12 for Sonoff B1R2
#define DCK_PIN 15  // 14 for Sonoff B1R2

class My9231Lamp {
 public:
  My9231Lamp()
      : mRed(0), mGreen(0), mBlue(0), mWarm(0), mCool(0) {
    _setup();
  }

  void setRed(uint8_t red) {
    mRed = red;
  }

  void setGreen(uint8_t green) {
    mGreen = green;
  }

  void setBlue(uint8_t blue) {
    mBlue = blue;
  }

  void setWarm(uint8_t warm) {
    mWarm = warm;
  }

  void setCool(uint8_t cool) {
    mCool = cool;
  }

  void set(uint8_t red, uint8_t green, uint8_t blue, uint8_t warm, uint8_t cool) {
    mRed = red;
    mGreen = green;
    mBlue = blue;
    mWarm = warm;
    mCool = cool;
  }

  void update() {
    _write(mRed, mGreen, mBlue, mWarm, mCool);
  }

  void off() {
    _write(0, 0, 0, 0, 0);
  }

 private:
  void _pulseDI(uint8_t times) {
    for (uint8_t i = 0; i < times; i++) {
      digitalWrite(DI_PIN, HIGH);
      digitalWrite(DI_PIN, LOW);
    }
  }

  void _pulseDCK(uint8_t times) {
    for (uint8_t i = 0; i < times; i++) {
      digitalWrite(DCK_PIN, HIGH);
      digitalWrite(DCK_PIN, LOW);
    }
  }

  void _writeData(uint8_t data) {
    // Send 8-bit data.
    for (uint8_t i = 0; i < 4; i++) {
      digitalWrite(DCK_PIN, LOW);
      digitalWrite(DI_PIN, (data & 0x80));
      digitalWrite(DCK_PIN, HIGH);
      data = data << 1;
      digitalWrite(DI_PIN, (data & 0x80));
      digitalWrite(DCK_PIN, LOW);
      digitalWrite(DI_PIN, LOW);
      data = data << 1;
    }
  }

  void _setup() {
    // GPIO setup.
    pinMode(DI_PIN, OUTPUT);
    pinMode(DCK_PIN, OUTPUT);

    _pulseDCK(64);                // Clear all duty registers (2 chipes * 32).
    delayMicroseconds(DELAY_US);  // TStop > 12us.
    // Send 12 DI pulse, after 6 pulse's falling edge store duty data, and 12
    // pulse's rising edge convert to command mode.
    _pulseDI(12);
    delayMicroseconds(DELAY_US);  // Delay >12us, begin send CMD data.
    // Send CMD data
    for (uint8_t n = 0; n < 2; n++) {  // 2 chips in SONOFF B1.
      _writeData(0x18);                // ONE_SHOT_DISABLE, REACTION_FAST, BIT_WIDTH_8, FREQUENCY_DIVIDE_1, SCATTER_APDM
    }
    delayMicroseconds(DELAY_US);  // TStart > 12us. Delay 12 us.
    // Send 16 DI pulse, at 14 pulse's falling edge store CMD data, and
    // at 16 pulse's falling edge convert to duty mode.
    _pulseDI(16);
    delayMicroseconds(DELAY_US);  // TStop > 12us.
  }

  void _write(uint8_t r, uint8_t g, uint8_t b, uint8_t w, uint8_t c) {
    // uint8_t duty[6] = {c, w, 0, g, r, b};  // RGBWC channels for Sonoff B1R2
    uint8_t duty[6] = {r, g, b, w, c, 0};  // RGBWC channels.

    delayMicroseconds(DELAY_US);  // TStop > 12us.
    for (uint8_t channel = 0; channel < 6; channel++) {
      _writeData(duty[channel]);  // Send 8-bit Data.
      delayMicroseconds(5);       // Delay 5us. Just in case there are artifacts
    }
    delayMicroseconds(DELAY_US);  // TStart > 12us. Ready for send DI pulse.
    _pulseDI(8);                  // Send 8 DI pulse. After 8 pulse falling edge, store old data.
    delayMicroseconds(DELAY_US);  // TStop > 12us.
  }

  uint8_t mRed;
  uint8_t mGreen;
  uint8_t mBlue;
  uint8_t mWarm;
  uint8_t mCool;
};
