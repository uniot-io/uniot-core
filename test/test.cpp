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

#include <Arduino.h>
#include <Board-WittyCloud.h>

#include "test_data_bytes.h"
#include "test_data_cbor.h"
#include "test_data_lisp.h"

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

void process()
{
  UNITY_BEGIN();

  // test_data_bytes.h
  RUN_TEST(test_function_bytes_terminate);
  // test_data_cbor.h
  RUN_TEST(test_function_cbor_read_string);
  RUN_TEST(test_function_cbor_read_int);
  RUN_TEST(test_function_cbor_put);
  // test_data_lisp.h
  RUN_TEST(test_function_lisp_simple);
  RUN_TEST(test_function_lisp_primitive);
  RUN_TEST(test_function_lisp_full_cycle);

  UNITY_END();
}

void setup()
{
  // NOTE!!! Wait for >2 secs
  // if board doesn't support software reset via Serial.DTR/RTS
  delay(2000);

  pinMode(RED, OUTPUT);
  pinMode(BLUE, OUTPUT);

  process();
}

void loop()
{
  digitalWrite(RED, HIGH);
  digitalWrite(BLUE, HIGH);
  delay(100);
  digitalWrite(RED, LOW);
  digitalWrite(BLUE, LOW);
  delay(500);
}
