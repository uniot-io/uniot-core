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
#include <Logger.h>

using namespace uniot;

void test_function_lisp_primitive_dwrite(void)
{
  unLisp::getInstance().pushPrimitive(globalPrimitive(dwrite));

  unLisp::getInstance().runCode("(dwrite 2 #t) (dwrite 0 #t)");
  unLisp::getInstance().popOutput();
  auto result = unLisp::getInstance().popOutput();

  TEST_ASSERT_EQUAL_STRING("#t", result.c_str());
}

void test_function_lisp_primitive_dread(void)
{
  unLisp::getInstance().pushPrimitive(globalPrimitive(dread));

  unLisp::getInstance().runCode("(dread 2)");
  auto result = unLisp::getInstance().popOutput();

  TEST_ASSERT_EQUAL_STRING("#t", result.c_str());
}

void test_function_lisp_primitive_awrite(void)
{
  unLisp::getInstance().pushPrimitive(globalPrimitive(awrite));

  unLisp::getInstance().runCode("(awrite 1 50)");
  auto result = unLisp::getInstance().popOutput();

  TEST_ASSERT_EQUAL_STRING("50", result.c_str());
}

void test_function_lisp_primitive_aread(void)
{
  unLisp::getInstance().pushPrimitive(globalPrimitive(aread));

  unLisp::getInstance().runCode("(aread 0)");
  auto result = unLisp::getInstance().popOutput();

  UNIOT_LOG_INFO("analog value: %s", result.c_str());
  TEST_ASSERT_GREATER_THAN(0, result.length());
}
