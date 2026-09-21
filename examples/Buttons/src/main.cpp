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
#define PIN_BUTTON 4        // button on the module board
#define PIN_BUTTON_FLASH 0  // flash button on the programmer board
#define PIN_RED 15
#define PIN_GREEN 12
#define PIN_BLUE 13

#define BTN_PIN_LEVEL LOW
#define LED_PIN_LEVEL HIGH

void setup() {
  Uniot.configWiFiStatusLed(PIN_RED, LED_PIN_LEVEL);

  // The reset button is a Lisp button too. It is linked during begin(), so it takes index 0
  // as long as every other button is added after that.
  //
  // Its clicks also drive the WiFi reset gesture: four or more clicks followed by a three
  // second hold clears the stored credentials, and a hold on its own reconnects.
  Uniot.configWiFiResetButton(PIN_BUTTON, BTN_PIN_LEVEL);
  Uniot.configWiFiResetOnReboot();

  // Register GPIO pins for Lisp access. Both button pins are readable with dread, which
  // gives a script the raw level alongside bclicked.
  Uniot.registerLispDigitalOutput(PIN_RED, PIN_GREEN, PIN_BLUE);
  Uniot.registerLispDigitalInput(PIN_BUTTON, PIN_BUTTON_FLASH);

  // A press nobody read stays pending for a while, so a script starting just after a press
  // would react to it. Clearing the buttons as a script starts drops those.
  Uniot.setLispStartHook([](LispStartReason) {
    Uniot.resetLispButtons();
  });

  Uniot.begin();

  // Added after begin(), so it follows the reset button in the bclicked register.
  auto flash = Uniot.addLispButton(PIN_BUTTON_FLASH, BTN_PIN_LEVEL, FOURCC(flsh),
                                   [](Button *, Button::Event event) {
                                     UNIOT_LOG_INFO("flash button: %s", event == Button::CLICK ? "click" : "long press");
                                   });

  UNIOT_LOG_INFO("CHIP_ID: %s", String(ESP.getChipId(), HEX).c_str());
  UNIOT_LOG_INFO("buttons: (bclicked 0) = board, (bclicked %d) = flash", flash);
}

void loop() {
  Uniot.loop();
}
