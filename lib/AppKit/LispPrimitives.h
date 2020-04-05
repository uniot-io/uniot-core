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
#include <PinMap.h>

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

  if (pin >= 0 && pin < UniotPinMap.getDigitalOutputLength())
    digitalWrite(UniotPinMap.getDigitalOutput(pin), state);
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

  if (pin >= 0 && pin < UniotPinMap.getDigitalInputLength())
    state = digitalRead(UniotPinMap.getDigitalInput(pin));
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

  if (pin >= 0 && pin < UniotPinMap.getAnalogOutputLength())
    analogWrite(UniotPinMap.getAnalogOutput(pin), value);
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

  if (pin >= 0 && pin < UniotPinMap.getAnalogInputLength())
    value = analogRead(UniotPinMap.getAnalogInput(pin));
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
