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

#include "RegisterManager.h"

namespace uniot {

/**
 * @brief Proxy class for RegisterManager that provides scoped access to a named register.
 * @defgroup registers_proxy Register Manager Proxy
 * @ingroup registers
 * @{
 *
 * This class acts as a proxy to the RegisterManager by binding operations to a specific
 * named register. This simplifies access to registers by removing the need to specify
 * the register name for each operation.
 */
class RegisterManagerProxy {
 public:
  /**
   * @brief Copy constructor is deleted to prevent unexpected behavior.
   */
  RegisterManagerProxy(const RegisterManagerProxy &) = delete;

  /**
   * @brief Assignment operator is deleted to prevent unexpected behavior.
   */
  void operator=(const RegisterManagerProxy &) = delete;

  /**
   * @brief Creates a new proxy for accessing a specific named register.
   *
   * @param name The register name to bind this proxy to
   * @param reg Pointer to the RegisterManager instance
   */
  RegisterManagerProxy(const String &name, RegisterManager *reg)
      : mName(name), mpRegister(reg) {
  }

  /**
   * @brief Gets a GPIO value from the specified index in the bound register.
   *
   * @param index The index within the register to read from
   * @param outValue Reference to store the read GPIO value
   * @retval true If the value was successfully read
   * @retval false If the value could not be read or index is invalid
   */
  bool getGpio(size_t index, uint8_t &outValue) const {
    return mpRegister->getGpio(mName, index, outValue);
  }

  /**
   * @brief Gets an object of the specified type from the bound register.
   *
   * @tparam T The type of object to retrieve
   * @param index The index within the register to get the object from
   * @retval T* Pointer to the requested object or nullptr if not found
   */
  template <typename T>
  T *getObject(size_t index) {
    return mpRegister->getObject<T>(mName, index);
  }

 private:
  String mName;         ///< The register name this proxy is bound to
  RegisterManager *mpRegister;  ///< Pointer to the underlying RegisterManager
};
/** @} */
}  // namespace uniot
