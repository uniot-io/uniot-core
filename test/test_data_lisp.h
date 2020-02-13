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
#include <UserPrimitive.h>
#include <Broker.h>
#include <CallbackSubscriber.h>

using namespace uniot;

// (simple_plus <integer> <integer>)
static Obj *user_prim_simple_plus(void *root, Obj **env, Obj **list)
{
  Obj *args = eval_list(root, env, list);
  if (length(args) != 2)
    error("Malformed simple_plus");
  Obj *x = args->car;
  Obj *y = args->cdr->car;
  if (x->type != TINT || y->type != TINT)
    error("simple_plus takes only numbers");
  return make_int(root, x->value + y->value);
}

void test_function_lisp_simple(void)
{
  unLisp::getInstance().runCode("(+ 1 2)");
  auto result = unLisp::getInstance().popOutput();

  TEST_ASSERT_EQUAL_STRING("3", result.c_str());
}

void test_function_lisp_native_primitive(void)
{
  unLisp::getInstance().pushPrimitive("simple_plus", user_prim_simple_plus);
  unLisp::getInstance().runCode("(simple_plus 2 3)");
  auto result = unLisp::getInstance().popOutput();

  TEST_ASSERT_EQUAL_STRING("5", result.c_str());
}

void test_function_lisp_user_primitive(void)
{
  unLisp::getInstance().pushPrimitive("user", [](Root root, VarObject env, VarObject list) {
    UserPrimitive primitive("user", root, env, list);
    primitive.assertArgs(3, BoolInt, Int, Bool);

    return primitive.makeBool(true);
  });

  unLisp::getInstance().runCode("(user 2 3 #t)");
  auto result = unLisp::getInstance().popOutput();

  TEST_ASSERT_EQUAL_STRING("#t", result.c_str());
}

void test_function_lisp_full_cycle(void)
{
  // test variables
  String lastLispResult = "?";

  // env global variables
  TaskScheduler scheduler;
  GeneralBroker broker;

  // simple lisp output subscriber
  GeneralCallbackSubscriber subscriber([&](int topic, int msg) {
    lastLispResult = unLisp::getInstance().popOutput();
  });

  // subscribe to the topic and connect subscriber to broker
  broker.connect(subscriber.subscribe(unLisp::OUTPUT_BUF));

  // connect publisher to broker
  broker.connect(&unLisp::getInstance());

  // push unLisp task to scheduler, attachment is regulated by the internal logic of the class
  scheduler.push(unLisp::getInstance().getTask());

  // create a broker task, push it to the scheduler and attach it manually
  auto brokerTask = TaskScheduler::make(&broker);
  scheduler.push(brokerTask);
  brokerTask->attach(1);

  // push user primitive to unLisp
  unLisp::getInstance().pushPrimitive("simple_plus", user_prim_simple_plus);
  // finally execute the code
  unLisp::getInstance().runCode("(task 1 10 '(simple_plus 5 2))");

  // here is emulation of main loop
  for (auto i = 0; i < 10000; i++)
  {
    scheduler.execute();
  }

  TEST_ASSERT_EQUAL_STRING("7", lastLispResult.c_str());
}
