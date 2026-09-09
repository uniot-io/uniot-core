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

#define PIN_STATUS_LED 13
#define PIN_RELAY 12
#define PIN_BUTTON 0
#define BTN_PIN_LEVEL LOW

void setup() {
  // Configure WiFi with status LED and reset button
  Uniot.configWiFiStatusLed(PIN_STATUS_LED);
  Uniot.configWiFiResetButton(PIN_BUTTON, BTN_PIN_LEVEL);

  // Reboot-reset is off unless asked for. Power-cycling clears the credentials, which
  // is a second way back when the button is not reachable.
  Uniot.configWiFiResetOnReboot();

  // Register GPIO pins for Lisp access
  Uniot.registerLispDigitalOutput(PIN_RELAY, PIN_STATUS_LED);

  // Create periodic tasks for monitoring
  Uniot.setInterval([]() {
    UNIOT_LOG_DEBUG("Free heap: %u", ESP.getFreeHeap());
  }, 5000);

  Uniot.setInterval([]() {
    UNIOT_LOG_DEBUG("Time: %s", Date::getFormattedTime().c_str());
  }, 5000);

  // Initialize and start Uniot
  Uniot.begin();
}

void loop() {
  Uniot.loop();
}
