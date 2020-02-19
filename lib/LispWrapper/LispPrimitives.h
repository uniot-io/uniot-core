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

#include <Board-WittyCloud.h>

#include <unLisp.h>
#include <PrimitiveExpeditor.h>

namespace uniot
{

Object user_prim_led(Root root, VarObject env, VarObject list)
{
  PrimitiveExpeditor expiditor("led", root, env, list);
  expiditor.assertArgs(2, Lisp::Int, Lisp::BoolInt);
  auto color = expiditor.getArgInt(0);
  auto state = expiditor.getArgBool(1);

  uint8_t pin = 0;
  switch (color)
  {
  case 0:
    pin = RED;
    break;
  case 1:
    pin = GREEN;
    break;
  case 2:
    pin = BLUE;
    break;
  default:
    expiditor.terminate("Out of range: color: [0, 1, 2]");
  }

  digitalWrite(pin, state);
  return expiditor.makeBool(state);
}

} // namespace uniot
