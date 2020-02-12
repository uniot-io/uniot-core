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

#include <unity.h>
#include <Arduino.h>
#include <Board-WittyCloud.h>
#include <CBOR.h>
#include "test_data_cbor.h"

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

void test_function_bytes_terminate(void)
{
  const unsigned char raw[] = {0x6F, 0x62, 0x6A, 0x65, 0x63, 0x74}; // "object"
  Bytes bytes(raw, sizeof(raw));
  TEST_ASSERT_EQUAL_MEMORY("object", bytes.terminate().c_str(), sizeof(raw) + 1);
}

void test_function_cbor_read_string(void)
{
  auto bytes = Bytes(cbor_object_1, sizeof(cbor_object_1));
  uniot::CBOR cbor(bytes);
  TEST_ASSERT_EQUAL_STRING("simple", cbor.getString("object").c_str());
}

void test_function_cbor_read_int(void)
{
  auto bytes = Bytes(cbor_object_1, sizeof(cbor_object_1));
  uniot::CBOR cbor(bytes);
  TEST_ASSERT_EQUAL(42, cbor.getInt("number"));
}

void test_function_cbor_put(void)
{
  uniot::CBOR cbor;
  cbor.put("object", "simple");
  cbor.put("number", 42);
  TEST_ASSERT_EQUAL_MEMORY(cbor_object_1, cbor.build().raw(), sizeof(cbor_object_1));
}

void process()
{
  UNITY_BEGIN();
  RUN_TEST(test_function_bytes_terminate);
  RUN_TEST(test_function_cbor_read_string);
  RUN_TEST(test_function_cbor_read_int);
  RUN_TEST(test_function_cbor_put);
  UNITY_END();
}

void setup()
{
  // NOTE!!! Wait for >2 secs
  // if board doesn't support software reset via Serial.DTR/RTS
  delay(2000);

  pinMode(RED, OUTPUT);

  process();
}

void loop()
{
  digitalWrite(RED, HIGH);
  delay(100);
  digitalWrite(RED, LOW);
  delay(500);
}