/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2024 Uniot <contact@uniot.io>
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

/** @cond */
/**
 * DO NOT DELETE THE "registers" GROUP DEFINITION BELOW.
 * Used to create the Named Registers topic in the documentation. If you want to delete this file,
 * please paste the group definition into another utility and delete this one.
 */
/** @endcond */

/**
 * @defgroup registers Named Registers
 * @brief Named registers for storing and managing collections of values
 */

#pragma once

#include <Arduino.h>

#include "Array.h"
#include "Map.h"

#ifdef __cplusplus
#include <functional>
#endif

namespace uniot {

/**
 * @brief A generalized Register class template that stores named collections of typed values.
 * @defgroup registers_register Register
 * @ingroup registers
 * @{
 *
 * The Register class provides a flexible data storage mechanism similar to a named registry or dictionary
 * where each entry (register) can contain multiple values of the same type. It's designed to be memory
 * efficient for embedded systems while providing a convenient interface for storing and retrieving data.
 *
 * @tparam T The type of values to store in the registers (can be any primitive or complex type)
 */
template <typename T>
class Register {
 public:
  /**
   * @brief Function type used for iterating through registers
   *
   * The callback receives the register name and a pointer to its value array
   */
  using IteratorCallback = std::function<void(const String&, SharedPointer<Array<T>>)>;

  /**
   * @brief Deleted copy constructor to prevent unintended copying
   */
  Register(Register const&) = delete;

  /**
   * @brief Deleted assignment operator to prevent unintended copying
   */
  void operator=(Register const&) = delete;

  /**
   * @brief Constructs an empty Register.
   */
  Register()
      : mRegisterMap() {}

  /**
   * @brief Sets or replaces a register with the given name and array of values.
   *
   * This method completely replaces any existing register with the same name.
   * If a register with the given name exists, it will be removed before adding the new one.
   * If count is 0, the register will be removed without creating a new one.
   *
   * @param name The name identifier for the register
   * @param values Pointer to an array of values to store
   * @param count Number of values in the array
   * @retval true Register was set successfully
   * @retval false Invalid parameters or memory allocation failed
   */
  bool setRegister(const String& name, const T* values, size_t count) {
    if (!values && count > 0) {
      return false;
    }

    mRegisterMap.remove(name);

    if (count > 0) {
      auto newArray = MakeShared<Array<T>>(count, values);
      if ((*newArray).size() != count) {
        return false;
      }
      mRegisterMap.put(name, std::move(newArray));
      for (size_t i = 0; i < count; ++i) {
        _processRegister(name, values[i]);
      }
    }

    return true;
  }

  /**
   * @brief Adds a single value to an existing register or creates a new one.
   *
   * If the register doesn't exist yet, it will be created with a default initial capacity.
   * The method automatically handles dynamic resizing if the register's capacity is exceeded.
   * After adding the value, the _processRegister hook is called to allow derived classes
   * to perform additional processing.
   *
   * @param name The name identifier for the register
   * @param value The value to add to the register
   * @retval true Value was added successfully
   * @retval false Memory allocation failed or register could not be created
   */
  bool addToRegister(const String& name, const T& value) {
    SharedPointer<Array<T>> reg = mRegisterMap.get(name, nullptr);
    if (!reg) {
      reg = MakeShared<Array<T>>(4);  // Default capacity of 4
      mRegisterMap.put(name, reg);
    }
    if (reg->push(value)) {
      _processRegister(name, value);
      return true;
    }
    return false;
  }

  /**
   * @brief Retrieves a value from the register by name and index.
   *
   * Performs bounds checking to ensure the requested index is valid.
   * The value is copied to the provided outValue reference if found.
   *
   * @param name The name identifier for the register
   * @param idx Zero-based index of the value within the register
   * @param outValue Reference where the retrieved value will be stored
   * @retval true Value was retrieved successfully
   * @retval false Register doesn't exist or index is out of bounds
   */
  bool getRegisterValue(const String& name, size_t idx, T& outValue) const {
    auto reg = mRegisterMap.get(name, nullptr);
    if (reg) {
      return reg->get(idx, outValue);
    }
    return false;
  }

  /**
   * @brief Updates a value in the register at the specified index.
   *
   * Performs bounds checking to ensure the requested index is valid.
   * After setting the value, the _processRegister hook is called to allow
   * derived classes to perform additional processing.
   *
   * @param name The name identifier for the register
   * @param idx Zero-based index of the value to update
   * @param value The new value to set
   * @retval true Value was updated successfully
   * @retval false Register doesn't exist or index is out of bounds
   */
  bool setRegisterValue(const String& name, size_t idx, const T& value) {
    auto reg = mRegisterMap.get(name, nullptr);
    if (reg && reg->set(idx, value)) {
      _processRegister(name, value);
      return true;
    }
    return false;
  }

  /**
   * @brief Gets the number of values in the specified register.
   *
   * @param name The name identifier for the register
   * @retval size_t The number of values in the register
   * @retval 0 The register doesn't exist or is empty
   */
  size_t getRegisterLength(const String& name) const {
    auto reg = mRegisterMap.get(name, nullptr);
    if (reg) {
      return reg->size();
    }
    return 0;
  }

  /**
   * @brief Iterates through all registers and calls the callback function for each one.
   *
   * This method provides a way to access all registers without knowing their names in advance.
   * The callback receives both the register name and a shared pointer to its array of values.
   *
   * Example usage:
   * ```
   * register.iterateRegisters([](const String& name, SharedPointer<Array<int>> values) {
   *     Serial.printf("Register %s has %d values\n", name.c_str(), values->size());
   * });
   * ```
   *
   * @param callback The function to call for each register (nullptr callbacks are ignored)
   */
  void iterateRegisters(IteratorCallback callback) const {
    if (!callback) return;

    mRegisterMap.begin();
    while (!mRegisterMap.isEnd()) {
      auto& item = mRegisterMap.current();
      callback(item.first, item.second);
      mRegisterMap.next();
    }
  }

 protected:
  /**
   * @brief Hook method called when a register value is added or modified.
   *
   * This virtual method is intended to be overridden by derived classes to implement
   * custom behavior when register values change. The base implementation does nothing.
   *
   * @param name The name identifier of the register being modified
   * @param value The value being set or added
   */
  virtual void _processRegister(const String& name, const T& value) {}

 private:
  /**
   * @brief Storage for all registers and their values
   */
  Map<String, SharedPointer<Array<T>>> mRegisterMap;
};
/** @} */
}  // namespace uniot
