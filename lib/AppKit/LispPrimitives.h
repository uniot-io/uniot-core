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

#include "Button.h"
#include "DefaultPrimitives.h"
#include "LispHelper.h"
#include "PrimitiveExpeditor.h"

/**
 * @namespace uniot::primitive
 * @brief Contains descriptions and implementations of primitive functions for hardware interaction.
 */
namespace uniot::primitive {
/**
 * @brief Contains Lisp primitive functions for hardware interaction.
 * @defgroup app-kit-primitives Primitives
 * @ingroup app-kit
 * @{
 *
 * Writes a digital value (HIGH/LOW) to the specified GPIO pin.
 * Takes two arguments: pin index and boolean state.
 * Returns the boolean state that was set.
 * Throws an error if the pin is out of valid range.
 *
 * @param root The Lisp environment root
 * @param env The current environment object
 * @param list List of arguments (pin number and state)
 * @retval Object Boolean value representing the set state
 */
Object dwrite(Root root, VarObject env, VarObject list) {
  auto expeditor = PrimitiveExpeditor::describe(name::dwrite, Lisp::Bool, 2, Lisp::Int, Lisp::BoolInt)
                       .init(root, env, list);
  expeditor.assertDescribedArgs();
  auto pin = expeditor.getArgInt(0);
  auto state = expeditor.getArgBool(1);

  uint8_t gpio = 0;
  if (pin >= 0 && expeditor.getAssignedRegister().getGpio(pin, gpio)) {
    digitalWrite(gpio, state);
  } else {
    expeditor.terminate("pin is out of range");
  }

  return expeditor.makeBool(state);
}

/**
 * @brief Digital read primitive function for reading pin input state.
 *
 * Reads a digital value (HIGH/LOW) from the specified GPIO pin.
 * Takes one argument: pin index.
 * Returns the boolean state that was read.
 * Throws an error if the pin is out of valid range.
 *
 * @param root The Lisp environment root
 * @param env The current environment object
 * @param list List of arguments (pin number)
 * @retval Object Boolean value representing the read state
 */
Object dread(Root root, VarObject env, VarObject list) {
  auto expeditor = PrimitiveExpeditor::describe(name::dread, Lisp::Bool, 1, Lisp::Int)
                       .init(root, env, list);
  expeditor.assertDescribedArgs();
  auto pin = expeditor.getArgInt(0);
  int state = 0;

  uint8_t gpio = 0;
  if (pin >= 0 && expeditor.getAssignedRegister().getGpio(pin, gpio)) {
    state = digitalRead(gpio);
  } else {
    expeditor.terminate("pin is out of range");
  }

  return expeditor.makeBool(state);
}

/**
 * @brief Analog write primitive function for setting PWM output.
 *
 * Writes an analog value (PWM) to the specified GPIO pin.
 * Takes two arguments: pin index and integer value (0-1023).
 * Returns the integer value that was set.
 * Throws an error if the pin is out of valid range.
 *
 * @param root The Lisp environment root
 * @param env The current environment object
 * @param list List of arguments (pin number and value)
 * @retval Object Integer value representing the set PWM value
 */
Object awrite(Root root, VarObject env, VarObject list) {
  auto expeditor = PrimitiveExpeditor::describe(name::awrite, Lisp::Int, 2, Lisp::Int, Lisp::Int)
                       .init(root, env, list);
  expeditor.assertDescribedArgs();
  auto pin = expeditor.getArgInt(0);
  auto value = expeditor.getArgInt(1);

  uint8_t gpio = 0;
  if (pin >= 0 && expeditor.getAssignedRegister().getGpio(pin, gpio)) {
    analogWrite(gpio, value);
  } else {
    expeditor.terminate("pin is out of range");
  }

  return expeditor.makeInt(value);
}

/**
 * @brief Analog read primitive function for reading analog input.
 *
 * Reads an analog value from the specified GPIO pin.
 * Takes one argument: pin index.
 * Returns the integer value that was read (0-1023).
 * Throws an error if the pin is out of valid range.
 *
 * @param root The Lisp environment root
 * @param env The current environment object
 * @param list List of arguments (pin number)
 * @retval Object Integer value representing the read analog value
 */
Object aread(Root root, VarObject env, VarObject list) {
  auto expeditor = PrimitiveExpeditor::describe(name::aread, Lisp::Int, 1, Lisp::Int)
                       .init(root, env, list);
  expeditor.assertDescribedArgs();
  auto pin = expeditor.getArgInt(0);
  int value = 0;

  uint8_t gpio = 0;
  if (pin >= 0 && expeditor.getAssignedRegister().getGpio(pin, gpio)) {
    value = analogRead(gpio);
  } else {
    expeditor.terminate("pin is out of range");
  }

  return expeditor.makeInt(value);
}

/**
 * @brief Button clicked primitive function for checking button click status.
 *
 * Checks if a button with the specified ID has been clicked.
 * Takes one argument: button ID.
 * Returns true if the button was clicked (and resets the click status).
 * Throws an error if the button ID is invalid.
 *
 * @param root The Lisp environment root
 * @param env The current environment object
 * @param list List of arguments (button ID)
 * @retval Object Boolean value indicating if button was clicked
 */
Object bclicked(Root root, VarObject env, VarObject list) {
  auto expeditor = PrimitiveExpeditor::describe(name::bclicked, Lisp::Bool, 1, Lisp::Int)
                       .init(root, env, list);
  expeditor.assertDescribedArgs();
  auto btnId = expeditor.getArgInt(0);
  bool value = false;

  auto btn = expeditor.getAssignedRegister().getObject<Button>(btnId);

  if (btn) {
    value = btn->resetClick();
  } else {
    expeditor.terminate("wrong button id");
  }

  return expeditor.makeBool(value);
}
/** @} */
}  // namespace uniot::primitive
