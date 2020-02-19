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

#include <unity.h>

#include <unLisp.h>
#include <PrimitiveExpeditor.h>
#include <LispPrimitives.h>

using namespace uniot;

void test_function_lisp_primitive_led(void)
{
  unLisp::getInstance().pushPrimitive("led", user_prim_led);

  unLisp::getInstance().runCode("(led 2 #t) (led 0 #t)");
  unLisp::getInstance().popOutput();
  auto result = unLisp::getInstance().popOutput();

  TEST_ASSERT_EQUAL_STRING("#t", result.c_str());
}
