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

#include "Common.h"
#include "ObjectRegisterRecord.h"
#include "Register.h"

namespace uniot {

/**
 * @brief Type alias for ObjectRegisterRecord pointers for better readability
 * @ingroup registers_object_register
 */
using RecordPtr = ObjectRegisterRecord *;

/**
 * @brief A registry system for managing object references by name.
 * @defgroup registers_object_register Object Register
 * @ingroup registers
 * @{
 *
 * ObjectRegister extends the Register class to specifically handle object references.
 * It provides a mechanism to link object references with names and retrieve them
 * in a type-safe manner. The registry also handles dangling pointers by checking
 * if objects still exist before returning them.
 */
class ObjectRegister : public Register<Pair<uint32_t, RecordPtr>> {
 public:
  /**
   * @brief Default constructor
   */
  ObjectRegister() {}

  /**
   * @brief Virtual destructor
   */
  virtual ~ObjectRegister() {}

  /**
   * @brief Deleted copy constructor to prevent copying
   */
  ObjectRegister(ObjectRegister const &) = delete;

  /**
   * @brief Deleted assignment operator to prevent copying
   */
  void operator=(ObjectRegister const &) = delete;

  /**
   * @brief Links an object to a name in the register
   *
   * @param name The string identifier for the object
   * @param link Pointer to the ObjectRegisterRecord to be registered
   * @param id Optional numeric identifier for the object (default: FOURCC(____)
   * @retval true Object was linked successfully
   * @retval false Object could not be linked (e.g., memory allocation failed)
   */
  bool link(const String &name, RecordPtr link, uint32_t id = FOURCC(____)) {
    return addToRegister(name, MakePair(id, link));
  }

  /**
   * @brief Retrieves a registered object by name and index with type casting
   *
   * This method provides type-safe access to registered objects. It also performs
   * validity checks to ensure the returned pointer is still valid. If the referenced
   * object no longer exists, the entry is marked as dead and nullptr is returned.
   *
   * @tparam T The type to cast the object to
   * @param name The string identifier of the object
   * @param index The index of the object if multiple objects share the same name
   * @retval T* Pointer to the requested object
   * @retval nullptr Object is not found or is invalid
   */
  template <typename T>
  T *get(const String &name, size_t index) {
    Pair<uint32_t, RecordPtr> record;
    if (getRegisterValue(name, index, record)) {
      if (!record.second) {
        return nullptr;
      }
      if (ObjectRegisterRecord::exists(record.second)) {
        return Type::safeStaticCast<T>(record.second);
      }
      setRegisterValue(name, index, MakePair(FOURCC(dead), nullptr));
      UNIOT_LOG_DEBUG("record is dead [%s][%d]", name.c_str(), index);
    }
    return nullptr;
  }
};
/** @} */
}  // namespace uniot
