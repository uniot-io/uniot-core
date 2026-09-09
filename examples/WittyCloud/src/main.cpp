/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2025 Uniot <contact@uniot.io>
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

using namespace uniot;

// WittyCloud board pin definitions
#define PIN_BUTTON 4
#define PIN_RED 15
#define PIN_GREEN 12
#define PIN_BLUE 13
#define PIN_LDR A0

#define BTN_PIN_LEVEL LOW
#define LED_PIN_LEVEL HIGH

void setup() {
  // Configure WiFi with status LED and reset button
  Uniot.configWiFiStatusLed(PIN_RED, LED_PIN_LEVEL);
  Uniot.configWiFiResetButton(PIN_BUTTON, BTN_PIN_LEVEL);

  // Reboot-reset is off unless asked for. Power-cycling clears the credentials, which
  // is a second way back when the button is not reachable.
  Uniot.configWiFiResetOnReboot();

  // Register GPIO pins for Lisp access
  Uniot.registerLispDigitalOutput(PIN_RED, PIN_GREEN, PIN_BLUE);
  Uniot.registerLispDigitalInput(PIN_BUTTON);
  Uniot.registerLispAnalogOutput(PIN_RED, PIN_GREEN, PIN_BLUE);
  Uniot.registerLispAnalogInput(PIN_LDR);

  // Create periodic tasks for monitoring
  Uniot.setInterval([]() {
    UNIOT_LOG_DEBUG("Free heap: %u", ESP.getFreeHeap());
  }, 5000);

  Uniot.setInterval([]() {
    UNIOT_LOG_DEBUG("Time: %s", Date::getFormattedTime().c_str());
  }, 5000);

  // Initialize and start Uniot
  Uniot.begin();

  // Log device information. After begin(), which is what opens the log stream.
  UNIOT_LOG_INFO("CHIP_ID: %s", String(ESP.getChipId(), HEX).c_str());
}

void loop() {
  Uniot.loop();
}
