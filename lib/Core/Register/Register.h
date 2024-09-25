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

#pragma once

#include <Arduino.h>

#include "Array.h"
#include "Map.h"

#ifdef __cplusplus
#include <functional>
#endif

namespace uniot {

/**
 * @brief A generalized Register class template that can store any type of values using fixed-size arrays.
 *
 * @tparam T The type of values to store in the registers.
 */
template <typename T>
class Register {
 public:
  using IteratorCallback = std::function<void(const String&, SharedPointer<Array<T>>)>;

  Register(Register const&) = delete;
  void operator=(Register const&) = delete;

  /**
   * @brief Constructs an empty Register.
   */
  Register()
      : mRegisterMap() {}

  /**
   * @brief Sets the register with the given name and array of values.
   *
   * @param name The name of the register.
   * @param values Pointer to array of values.
   * @param count Number of values.
   * @return true if the register was set successfully, false otherwise.
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
   * @brief Adds a value to an existing register. If the register doesn't exist, it will be created.
   *
   * @param name The name of the register.
   * @param value The value to add.
   * @return true if the value was added successfully, false otherwise.
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
   * @brief Retrieves a value from the register by name and index with bounds checking.
   *
   * @param name The name of the register.
   * @param idx The index of the value within the register.
   * @param outValue Reference to store the retrieved value.
   * @return true if the element was retrieved successfully, false otherwise.
   */
  bool getRegisterValue(const String& name, size_t idx, T& outValue) const {
    auto reg = mRegisterMap.get(name, nullptr);
    if (reg) {
      return reg->get(idx, outValue);
    }
    return false;
  }

  /**
   * @brief Sets a value in the register by name and index with bounds checking.
   *
   * @param name The name of the register.
   * @param idx The index of the value within the register.
   * @param value The value to set.
   * @return true if the element was set successfully, false otherwise.
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
   * @param name The name of the register.
   * @return The number of values, or 0 if the register does not exist.
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
   * @param callback The callback function to call for each register.
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
   * @brief Processes a register value. To be overridden by child classes for specific behaviors.
   *
   * @param name The name of the register.
   * @param value The value being set.
   */
  virtual void _processRegister(const String& name, const T& value) {}

 private:
  // Map from register name (String) to Array<T>
  Map<String, SharedPointer<Array<T>>> mRegisterMap;
};

}  // namespace uniot
