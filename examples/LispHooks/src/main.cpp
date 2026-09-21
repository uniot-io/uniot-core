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

// Switches the sensor's supply. On a bare ESP-12E module this is the on-board LED,
// which lights while the sensor is powered.
#define PIN_SENSOR_POWER 2
#define SENSOR_POWER_ON LOW
#define SENSOR_POWER_OFF HIGH

#define PIN_SENSOR A0

bool sensorPowered = false;

void sensorPowerOn() {
  digitalWrite(PIN_SENSOR_POWER, SENSOR_POWER_ON);
  sensorPowered = true;
}

void sensorPowerOff() {
  digitalWrite(PIN_SENSOR_POWER, SENSOR_POWER_OFF);
  sensorPowered = false;
}

const char *toString(LispStartReason reason) {
  switch (reason) {
    case LispStartReason::Restored:
      return "restored";
    case LispStartReason::Received:
      return "received";
  }
  return "unknown";
}

const char *toString(LispStopReason reason) {
  switch (reason) {
    case LispStopReason::Completed:
      return "completed";
    case LispStopReason::Replaced:
      return "replaced";
    case LispStopReason::Cleared:
      return "cleared";
    case LispStopReason::Failed:
      return "failed";
  }
  return "unknown";
}

// Reads the sensor from a script. It refuses to run while the sensor is unpowered,
// which the hooks below make impossible for as long as a script is running.
Object sensor_read(Root root, VarObject env, VarObject list) {
  auto expeditor = PrimitiveExpeditor::describe("sensor_read", Lisp::Int, 0)
                       .init(root, env, list);
  expeditor.assertDescribedArgs();

  if (!sensorPowered) {
    expeditor.terminate("sensor is not powered");
  }

  auto value = analogRead(PIN_SENSOR);
  UNIOT_LOG_INFO("sensor_read: %d", value);

  return expeditor.makeInt(value);
}

void setup() {
  pinMode(PIN_SENSOR_POWER, OUTPUT);
  sensorPowerOff();

  Uniot.configWiFiResetOnReboot();

  Uniot.addLispPrimitive(sensor_read);

  // Runs before the script is evaluated, so the sensor is ready for its first read.
  Uniot.setLispStartHook([](LispStartReason reason) {
    UNIOT_LOG_INFO("script started: %s", toString(reason));
    sensorPowerOn();
  });

  // Runs whenever the script stops, for any reason. Hooks must stay short, and a failed
  // script is stopped while the interpreter is still unwinding its error, so in that case
  // the hook only cuts the power and leaves the report to setImmediate().
  Uniot.setLispStopHook([](LispStopReason reason) {
    sensorPowerOff();

    if (reason == LispStopReason::Failed) {
      Uniot.setImmediate([]() {
        UNIOT_LOG_WARN("script stopped: failed");
      });
      return;
    }

    UNIOT_LOG_INFO("script stopped: %s", toString(reason));
  });

  Uniot.begin();
}

void loop() {
  Uniot.loop();
}
