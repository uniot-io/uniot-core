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

#include "CBORObject.h"
#include "GpioRegister.h"
#include "ObjectRegister.h"

namespace uniot {

/**
 * @brief Manages GPIO and Object registers.
 * @defgroup registers_manager Register Manager
 * @ingroup registers
 * @{
 *
 * The RegisterManager provides a unified interface for managing both GPIO pins and
 * arbitrary objects within named registers. It handles the registration, access, and
 * serialization of these resources.
 */
class RegisterManager {
 public:
  /**
   * @brief Default constructor
   */
  RegisterManager() {}

  /**
   * @brief Copy constructor is deleted to prevent unexpected behavior.
   */
  RegisterManager(RegisterManager const &) = delete;

  /**
   * @brief Assignment operator is deleted to prevent unexpected behavior.
   */
  void operator=(RegisterManager const &) = delete;

  /**
   * @brief Sets one or more pins as digital inputs.
   *
   * @tparam Args Variadic template for multiple pin numbers
   * @param first The first pin to configure
   * @param args Additional pins to configure
   */
  template <typename... Args>
  void setDigitalInput(uint8_t first, Args... args) {
    mGpioRegistry.setDigitalInput(first, args...);
  }

  /**
   * @brief Sets one or more pins as digital outputs.
   *
   * @tparam Args Variadic template for multiple pin numbers
   * @param first The first pin to configure
   * @param args Additional pins to configure
   */
  template <typename... Args>
  void setDigitalOutput(uint8_t first, Args... args) {
    mGpioRegistry.setDigitalOutput(first, args...);
  }

  /**
   * @brief Sets one or more pins as analog inputs.
   *
   * @tparam Args Variadic template for multiple pin numbers
   * @param first The first pin to configure
   * @param args Additional pins to configure
   */
  template <typename... Args>
  void setAnalogInput(uint8_t first, Args... args) {
    mGpioRegistry.setAnalogInput(first, args...);
  }

  /**
   * @brief Sets one or more pins as analog outputs.
   *
   * @tparam Args Variadic template for multiple pin numbers
   * @param first The first pin to configure
   * @param args Additional pins to configure
   */
  template <typename... Args>
  void setAnalogOutput(uint8_t first, Args... args) {
    mGpioRegistry.setAnalogOutput(first, args...);
  }

  /**
   * @brief Links an object to a named register.
   *
   * @param name The register name to link the object to
   * @param link Pointer to the object to link
   * @param id Optional identifier for the object (default: FOURCC(____)
   * @retval true If the link operation was successful
   * @retval false If the link operation failed
   */
  bool link(const String &name, RecordPtr link, uint32_t id = FOURCC(____)) {
    return mObjectRegistry.link(name, link, id);
  }

  /**
   * @brief Gets a GPIO value from a named register.
   *
   * @param name The register name to read from
   * @param index The index within the register to read
   * @param outValue Reference to store the read GPIO value
   * @retval true If the value was successfully read
   * @retval false If the value could not be read or parameters are invalid
   */
  bool getGpio(const String &name, size_t index, uint8_t &outValue) const {
    return mGpioRegistry.getRegisterValue(name, index, outValue);
  }

  /**
   * @brief Gets an object of the specified type from a named register.
   *
   * @tparam T The type of object to retrieve
   * @param name The register name to read from
   * @param index The index within the register to get the object from
   * @retval T* Pointer to the requested object or nullptr if not found
   */
  template <typename T>
  T *getObject(const String &name, size_t index) {
    return mObjectRegistry.get<T>(name, index);
  }

  /**
   * @brief Gets the number of elements in the specified named register.
   *
   * Checks both GPIO and Object registers for the specified name.
   *
   * @param name The register name to check
   * @retval size_t The number of elements in the register, or 0 if not found
   */
  size_t getRegisterLength(const String &name) const {
    auto length = mGpioRegistry.getRegisterLength(name);
    if (!length) {
      length = mObjectRegistry.getRegisterLength(name);
    }
    return length;
  }

  /**
   * @brief Serializes all registers (GPIO and Object) to a CBOR object.
   *
   * GPIO registers are serialized as arrays of values.
   * Object registers are serialized as arrays of their identifiers.
   *
   * @param obj The CBOR object to serialize the registers into
   */
  void serializeRegisters(CBORObject &obj) {
    mGpioRegistry.iterateRegisters([&](const String &name, SharedPointer<Array<uint8_t>> values) {
      obj.putArray(name.c_str()).append(values->size(), values->raw());
    });
    mObjectRegistry.iterateRegisters([&](const String &name, SharedPointer<Array<Pair<uint32_t, RecordPtr>>> values) {
      auto arr = obj.putArray(name.c_str());
      for (size_t i = 0; i < values->size(); i++) {
        auto &pair = (*values)[i];
        arr.append(pair.first);
      }
    });
  }

 private:
  ObjectRegister mObjectRegistry;  ///< Registry for managing object references
  GpioRegister mGpioRegistry;      ///< Registry for managing GPIO pins
};
/** @} */
}  // namespace uniot
