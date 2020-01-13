#pragma once

#include <ConsoleController.h>
#include <i2c_ssd1306.h>
#include "DPad.h"

#define DPAD_UP       D6
#define DPAD_CENTER   D5
#define DPAD_DOWN     D7
#define VIBRO         D0

#define OUT0          VIBRO

#define DPAD_PIN_MODE INPUT_PULLUP
#define VIBRO_LEVEL   LOW

#define SDA           D1
#define SCL           D2
