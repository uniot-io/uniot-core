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


/**
 * @file TypeId.h
 * @defgroup utils_typeid TypeId
 * @ingroup utils
 * @brief Provides a lightweight type identification system for runtime type checking and safe casting.
 *
 * This file defines a type identification mechanism that allows objects to expose their type
 * information at runtime, enabling type-safe downcasting without using RTTI.
 */


#pragma once

namespace uniot {

/**
 * @typedef const void* type_id
 * @brief Type alias for a unique type identifier.
 * @ingroup utils_typeid
 *
 * Each unique C++ type will have a distinct address used as its identifier,
 * allowing for efficient runtime type comparisons.
 */
typedef const void* type_id;

/**
 * @brief Interface for objects that expose their runtime type information.
 * @ingroup utils_typeid
 * @{
 *
 * Classes implementing this interface can be safely downcasted using Type::safeStaticCast.
 */
class IWithType {
 public:
  /**
   * @brief Get the runtime type identifier for this object.
   * @retval type_id The unique type identifier for this object.
   */
  virtual type_id getTypeId() const = 0;
};
/** @} */

/**
 * @brief Utility class providing static methods for runtime type identification and safe casting.
 * @ingroup utils_typeid
 * @{
 *
 * This class leverages the address of static variables to generate unique type identifiers
 * without requiring RTTI or additional memory overhead per instance.
 */
class Type {
 public:
  /**
   * @brief Get the unique type identifier for a given type T.
   *
   * This method creates a static variable whose address serves as a unique identifier
   * for the template parameter type T.
   *
   * @tparam T The type to get an identifier for.
   * @retval type_id The unique identifier for the type.
   */
  template <class T>
  static inline type_id getTypeId() {
    static T* TypeUniqueMarker = nullptr;
    return &TypeUniqueMarker;
  }

  /**
   * @brief Safely cast a pointer to type T if the runtime type matches.
   *
   * This method performs a runtime type check before casting, providing type-safe
   * downcasting without using dynamic_cast.
   *
   * @tparam T The target type to cast to.
   * @param obj Pointer to an object implementing IWithType.
   * @retval T* Pointer to the object as type T.
   * @retval nullptr The cast failed (the object is not of type T).
   */
  template <typename T>
  static inline T* safeStaticCast(IWithType* obj) {
    if (obj->getTypeId() == Type::getTypeId<T>()) {
      return static_cast<T*>(obj);
    }
    UNIOT_LOG_DEBUG("cast failed from [%lu] to [%lu]", obj->getTypeId(), Type::getTypeId<T>());
    return nullptr;
  }
};
/** @} */
}
