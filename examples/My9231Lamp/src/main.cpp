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
#include <My9231Lamp.h>

using namespace uniot;

My9231Lamp Lamp;

// Custom Lisp primitive to control the lamp
Object lamp_update(Root root, VarObject env, VarObject list) {
  auto expeditor = PrimitiveExpeditor::describe("lamp_update", Lisp::Bool, 5, Lisp::Int, Lisp::Int, Lisp::Int, Lisp::Int, Lisp::Int)
                       .init(root, env, list);
  expeditor.assertDescribedArgs();

  // Get RGB, warm white, and cool white values
  auto red = expeditor.getArgInt(0);
  auto green = expeditor.getArgInt(1);
  auto blue = expeditor.getArgInt(2);
  auto warm = expeditor.getArgInt(3);
  auto cool = expeditor.getArgInt(4);

  // Clamp values to valid range
  red = std::min(255, std::max(0, red));
  green = std::min(255, std::max(0, green));
  blue = std::min(255, std::max(0, blue));
  warm = std::min(255, std::max(0, warm));
  cool = std::min(255, std::max(0, cool));

  // Note: For Sonoff B1R2, you may need to adjust:
  // warm = warm > 0 ? 255 : 0;
  // cool = cool > 0 ? 255 : 0;

  // Update the lamp
  Lamp.off();
  Lamp.set(red, green, blue, warm, cool);
  Lamp.update();

  return expeditor.makeBool(true);
}

void setup() {
  // Initialize the lamp
  Lamp.off();

  // Custom WiFi status LED handler that uses the lamp
  Uniot.addWifiStatusLedListener([](bool state) {
    Lamp.off();
    Lamp.setRed(state ? 10 : 0);
    Lamp.update();
  });

  // Add custom Lisp primitive for lamp control
  Uniot.addLispPrimitive(lamp_update);

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
