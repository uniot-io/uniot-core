#pragma once

#define LDR A0
#define RED D8
#define GREEN D6
#define BLUE D7
#define LED D4
#define PIN_BUTTON D2

#define BTN_PIN_MODE INPUT

const uint8_t uniot_digital_pin_map[] = {D8, D6, D7};
const uint8_t uniot_analog_w_pin_map[] = {D8, D6, D7};
const uint8_t uniot_analog_r_pin_map[] = {A0};

#define UNIOT_DIGITAL_PIN_LENGTH 3
#define UNIOT_DIGITAL_PIN_MAP uniot_digital_pin_map

#define UNIOT_ANALOG_W_PIN_LENGTH 3
#define UNIOT_ANALOG_W_PIN_MAP uniot_analog_w_pin_map

#define UNIOT_ANALOG_R_PIN_LENGTH 1
#define UNIOT_ANALOG_R_PIN_MAP uniot_analog_r_pin_map
