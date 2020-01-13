#pragma once

#include <ConsoleController.h>
#include <i2c_scanner.h>
#include <i2c_ssd1306.h>
#include "DPad.h"

#define DPAD_UP       0
#define DPAD_CENTER   2
#define DPAD_DOWN     14
#define VIBRO         15
#define BATT          A0
#define OUT0          12
#define OUT1          13

#define DPAD_PIN_MODE INPUT
#define VIBRO_LEVEL   HIGH

#ifdef _BITS_DEBUG_
#define SDA           2
#define SCL           0
#else
#define SDA           5
#define SCL           4
#endif