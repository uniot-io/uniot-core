/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2025 Uniot <contact@uniot.io>
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

#include <Arduino.h>

/** @cond */
/**
 * DO NOT DELETE THE "json" GROUP DEFINITION BELOW.
 * Used to create the JSON Utilities topic in the documentation. If you want to delete this file,
 * please paste the group definition into another utility and delete this one.
 */
/** @endcond */

/**
 * @file MicroJSON.h
 * @defgroup json JSON
 * @brief Lightweight JSON generation utilities for Uniot devices
 * @ingroup utils
 * @{
 *
 * This file provides a minimal JSON generation library optimized for embedded
 * systems. It allows building JSON objects and arrays incrementally by writing
 * directly to a String buffer, minimizing memory allocation overhead.
 *
 * The library supports:
 * - JSON objects with key-value pairs
 * - JSON arrays with various value types
 * - Nested objects and arrays
 * - String and numeric value types
 * - Automatic comma and quote handling
 *
 * Example usage:
 * @code
 * String jsonBuffer;
 * uniot::JSON::Object root(jsonBuffer);
 * root.put("device", "esp32")
 *     .put("temperature", 25)
 *     .putArray("sensors").append("temp").append("humidity").close();
 * root.close();
 * // Result: {"device":"esp32","temperature":25,"sensors":["temp","humidity"]}
 * @endcode
 */

namespace uniot {
namespace JSON {

class Array;

/**
 * @brief JSON object builder for creating key-value pairs
 *
 * This class provides a fluent interface for building JSON objects by appending
 * key-value pairs to a String buffer. It automatically handles JSON formatting
 * including quotes, commas, and braces.
 */
class Object {
 public:
  /**
   * @brief Construct a new JSON Object
   * @param out Reference to the String buffer where JSON will be written
   */
  Object(String &out) : mOut(out), mFirst(true) {
    mOut += "{";
  }

  /**
   * @brief Add a string key-value pair to the object
   * @param key The key name
   * @param value The string value
   * @param quote Whether to quote the value (default: true)
   * @retval Object& Reference to this object for method chaining
   */
  inline Object &put(const String &key, const String &value, bool quote = true) {
    _begin(key);
    mOut += (quote ? "\"" + value + "\"" : value);
    return *this;
  }

  /**
   * @brief Add an integer key-value pair to the object
   * @param key The key name
   * @param value The integer value
   * @retval Object& Reference to this object for method chaining
   */
  inline Object &put(const String &key, int value) {
    return put(key, String(value), false);
  }

  /**
   * @brief Create a nested array for the given key
   * @param key The key name for the array
   * @retval Array New Array object for the nested array
   */
  inline Array putArray(const String &key);

  /**
   * @brief Create a nested object for the given key
   * @param key The key name for the nested object
   * @retval Object New Object for the nested object
   */
  inline Object putObject(const String &key);

  /**
   * @brief Close the JSON object by appending the closing brace
   */
  inline void close() {
    mOut += "}";
  }

 private:
  String &mOut;  ///< Reference to the output String buffer
  bool mFirst;   ///< Flag to track if this is the first key-value pair

  /**
   * @brief Internal method to handle comma separation and key formatting
   * @param key The key to add
   */
  inline void _begin(const String &key) {
    if (!mFirst) {
      mOut += ",";
    }
    mOut += "\"" + key + "\":";
    mFirst = false;
  }
};

/**
 * @brief JSON array builder for creating ordered lists of values
 *
 * This class provides a fluent interface for building JSON arrays by appending
 * values to a String buffer. It automatically handles JSON formatting including
 * quotes, commas, and brackets.
 */
class Array {
 public:
  /**
   * @brief Construct a new JSON Array
   * @param out Reference to the String buffer where JSON will be written
   */
  Array(String &out) : mOut(out), mFirst(true) {
    mOut += "[";
  }

  /**
   * @brief Append a string value to the array
   * @param value The string value to append
   * @param quote Whether to quote the value (default: true)
   * @retval Array& Reference to this array for method chaining
   */
  inline Array &append(const String &value, bool quote = true) {
    _begin();
    mOut += (quote ? "\"" + value + "\"" : value);
    return *this;
  }

  /**
   * @brief Append an integer value to the array
   * @param value The integer value to append
   * @retval Array& Reference to this array for method chaining
   */
  inline Array &append(int value) {
    return append(String(value), false);
  }

  /**
   * @brief Create a nested array as an element
   * @retval Array New Array object for the nested array
   */
  inline Array appendArray();

  /**
   * @brief Create a nested object as an element
   * @retval Object New Object for the nested object
   */
  Object appendObject();

  /**
   * @brief Close the JSON array by appending the closing bracket
   */
  inline void close() {
    mOut += "]";
  }

 private:
  String &mOut;  ///< Reference to the output String buffer
  bool mFirst;   ///< Flag to track if this is the first element

  /**
   * @brief Internal method to handle comma separation
   */
  inline void _begin() {
    if (!mFirst) mOut += ",";
    mFirst = false;
  }
};

inline Array Object::putArray(const String &key) {
  _begin(key);
  return Array(mOut);
}

inline Object Object::putObject(const String &key) {
  _begin(key);
  return Object(mOut);
}

inline Array Array::appendArray() {
  _begin();
  return Array(mOut);
}

inline Object Array::appendObject() {
  _begin();
  return Object(mOut);
}

}  // namespace JSON
}  // namespace uniot

/** @} */