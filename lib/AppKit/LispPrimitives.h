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

#include <Button.h>
#include <LispHelper.h>
#include <PinMap.h>
#include <PrimitiveExpeditor.h>

namespace uniot {
namespace primitive {
using namespace lisp;

Object dwrite(Root root, VarObject env, VarObject list) {
  auto expeditor = PrimitiveExpeditor::describe(getPrimitiveName(), Lisp::Bool, 2, Lisp::Int, Lisp::BoolInt)
                       .init(root, env, list);
  // PrimitiveExpeditor expeditor("ignore_name", root, env, list);
  expeditor.assertDescribedArgs();
  auto pin = expeditor.getArgInt(0);
  auto state = expeditor.getArgBool(1);

  if (pin >= 0 && pin < UniotPinMap.getDigitalOutputLength())
    digitalWrite(UniotPinMap.getDigitalOutput(pin), state);
  else
    expeditor.terminate("pin is out of range");

  return expeditor.makeBool(state);
}

Object dread(Root root, VarObject env, VarObject list) {
  auto expeditor = PrimitiveExpeditor::describe(getPrimitiveName(), Lisp::Bool, 1, Lisp::Int)
                       .init(root, env, list);
  expeditor.assertDescribedArgs();
  auto pin = expeditor.getArgInt(0);
  int state = 0;

  if (pin >= 0 && pin < UniotPinMap.getDigitalInputLength())
    state = digitalRead(UniotPinMap.getDigitalInput(pin));
  else
    expeditor.terminate("pin is out of range");

  return expeditor.makeBool(state);
}

Object awrite(Root root, VarObject env, VarObject list) {
  auto expeditor = PrimitiveExpeditor::describe(getPrimitiveName(), Lisp::Int, 2, Lisp::Int, Lisp::Int)
                       .init(root, env, list);
  expeditor.assertDescribedArgs();
  auto pin = expeditor.getArgInt(0);
  auto value = expeditor.getArgInt(1);

  if (pin >= 0 && pin < UniotPinMap.getAnalogOutputLength())
    analogWrite(UniotPinMap.getAnalogOutput(pin), value);
  else
    expeditor.terminate("pin is out of range");

  return expeditor.makeInt(value);
}

Object aread(Root root, VarObject env, VarObject list) {
  auto expeditor = PrimitiveExpeditor::describe(getPrimitiveName(), Lisp::Int, 1, Lisp::Int)
                       .init(root, env, list);
  expeditor.assertDescribedArgs();
  auto pin = expeditor.getArgInt(0);
  int value = 0;

  if (pin >= 0 && pin < UniotPinMap.getAnalogInputLength())
    value = analogRead(UniotPinMap.getAnalogInput(pin));
  else
    expeditor.terminate("pin is out of range");

  return expeditor.makeInt(value);
}

Object bclicked(Root root, VarObject env, VarObject list) {
  auto expeditor = PrimitiveExpeditor::describe(getPrimitiveName(), Lisp::Bool, 1, Lisp::Int)
                       .init(root, env, list);
  expeditor.assertDescribedArgs();
  auto btnId = expeditor.getArgInt(0);
  bool value = false;

  auto btn = expeditor.getCurrentRegister().first<Button>();
  if (btnId > 0) {
    for (int i = 1; i <= btnId; i++) {
      btn = expeditor.getCurrentRegister().next<Button>();
      if (!btn)
        break;
    }
  }

  if (btn)
    value = btn->resetClick();
  else
    expeditor.terminate("wrong button id");

  return expeditor.makeBool(value);
}

}  // namespace primitive
}  // namespace uniot
