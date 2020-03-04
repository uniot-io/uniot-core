/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2020 Uniot <contact@uniot.io>
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

#include <PrimitiveExpeditor.h>
#include <LispHelper.h>
#include <Button.h>

#if not defined(UNIOT_DIGITAL_PIN_MAP)
#error "You must define UNIOT_DIGITAL_PIN_MAP and UNIOT_DIGITAL_PIN_LENGTH to be able to use primitives such as 'dwrite' and 'dread'"
#elif not defined(UNIOT_DIGITAL_PIN_LENGTH)
#error "You must define UNIOT_DIGITAL_PIN_MAP and UNIOT_DIGITAL_PIN_LENGTH to be able to use primitives such as 'dwrite' and 'dread'"
#endif

#if not defined(UNIOT_ANALOG_W_PIN_MAP)
#error "You must define UNIOT_ANALOG_W_PIN_MAP and UNIOT_ANALOG_W_PIN_LENGTH to be able to use primitives such as 'awrite'"
#elif not defined(UNIOT_ANALOG_W_PIN_LENGTH)
#error "You must define UNIOT_ANALOG_W_PIN_MAP and UNIOT_ANALOG_W_PIN_LENGTH to be able to use primitives such as 'awrite'"
#endif

#if not defined(UNIOT_ANALOG_R_PIN_MAP)
#error "You must define UNIOT_ANALOG_R_PIN_MAP and UNIOT_ANALOG_R_PIN_LENGTH to be able to use primitives such as 'aread'"
#elif not defined(UNIOT_ANALOG_R_PIN_LENGTH)
#error "You must define UNIOT_ANALOG_R_PIN_MAP and UNIOT_ANALOG_R_PIN_LENGTH to be able to use primitives such as 'aread'"
#endif

namespace uniot
{
namespace primitive
{
using namespace lisp;

Object dwrite(Root root, VarObject env, VarObject list)
{
  exportPrimitiveNameTo(name);
  PrimitiveExpeditor expiditor(name, root, env, list);
  expiditor.assertArgs(2, Lisp::Int, Lisp::BoolInt);
  auto pin = expiditor.getArgInt(0);
  auto state = expiditor.getArgBool(1);

  if (pin >= 0 && pin < UNIOT_DIGITAL_PIN_LENGTH)
    digitalWrite(UNIOT_DIGITAL_PIN_MAP[pin], state);
  else
    expiditor.terminate("pin is out of range");

  return expiditor.makeBool(state);
}

Object dread(Root root, VarObject env, VarObject list)
{
  exportPrimitiveNameTo(name);
  PrimitiveExpeditor expiditor(name, root, env, list);
  expiditor.assertArgs(1, Lisp::Int);
  auto pin = expiditor.getArgInt(0);
  int state = 0;

  if (pin >= 0 && pin < UNIOT_DIGITAL_PIN_LENGTH)
    state = digitalRead(UNIOT_DIGITAL_PIN_MAP[pin]);
  else
    expiditor.terminate("pin is out of range");

  return expiditor.makeBool(state);
}

Object awrite(Root root, VarObject env, VarObject list)
{
  exportPrimitiveNameTo(name);
  PrimitiveExpeditor expiditor(name, root, env, list);
  expiditor.assertArgs(2, Lisp::Int, Lisp::Int);
  auto pin = expiditor.getArgInt(0);
  auto value = expiditor.getArgInt(1);

  if (pin >= 0 && pin < UNIOT_ANALOG_W_PIN_LENGTH)
    analogWrite(UNIOT_ANALOG_W_PIN_MAP[pin], value);
  else
    expiditor.terminate("pin is out of range");

  return expiditor.makeInt(value);
}

Object aread(Root root, VarObject env, VarObject list)
{
  exportPrimitiveNameTo(name);
  PrimitiveExpeditor expiditor(name, root, env, list);
  expiditor.assertArgs(1, Lisp::Int);
  auto pin = expiditor.getArgInt(0);
  int value = 0;

  if (pin >= 0 && pin < UNIOT_ANALOG_R_PIN_LENGTH)
    value = analogRead(UNIOT_ANALOG_R_PIN_MAP[pin]);
  else
    expiditor.terminate("pin is out of range");

  return expiditor.makeInt(value);
}

Object bclicked(Root root, VarObject env, VarObject list)
{
  exportPrimitiveNameTo(name);
  PrimitiveExpeditor expiditor(name, root, env, list);
  expiditor.assertArgs(1, Lisp::Int);
  auto btnId = expiditor.getArgInt(0);
  bool value = false;

  auto btn = expiditor.getCurrentRegister().first<Button>();
  if (btnId > 0)
  {
    for (int i = 1; i <= btnId; i++)
    {
      btn = expiditor.getCurrentRegister().next<Button>();
      if (!btn)
        break;
    }
  }

  if (btn)
    value = btn->resetClick();
  else
    expiditor.terminate("wrong button id");

  return expiditor.makeBool(value);
}

} // namespace primitive
} // namespace uniot
