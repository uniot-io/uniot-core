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

/**
 * @file LispHelper.h
 * @defgroup uniot-lisp-helper Helper
 * @ingroup uniot-lisp
 * @brief Helper utilities and type definitions for working with UniotLisp.
 *
 * This file provides wrapper types and utility functions to simplify
 * the usage of the UniotLisp interpreter within the Uniot Core.
 */

#pragma once

#include <libminilisp.h>

/**
 * @ingroup uniot-lisp-helper
 * @brief Macro to get the current function name.
 * @retval name The name of the current function.
 */
#define getPrimitiveName() (__func__)

/**
 * @ingroup uniot-lisp-helper
 * @brief Exports the current function name to a provided variable.
 * @param name The variable name that will hold the function name.
 */
#define exportPrimitiveNameTo(name) \
  char name[sizeof(__func__)];      \
  snprintf(name, sizeof(name), __func__)

namespace uniot {

/**
 * @namespace uniot::lisp
 * @brief Contains type definitions and utilities for interacting with the Lisp interpreter.
 */
namespace lisp {
/**
 * @typedef Object
 * @brief A pointer to a Lisp object structure.
 * @ingroup uniot-lisp-helper
 */
using Object = struct Obj *;

/**
 * @typedef VarObject
 * @brief A pointer to a pointer to a Lisp object structure.
 * @details Used for functions that may modify the Lisp object reference.
 * @ingroup uniot-lisp-helper
 */
using VarObject = struct Obj **;

/**
 * @typedef Root
 * @brief A generic pointer representing the root of a Lisp environment.
 * @ingroup uniot-lisp-helper
 */
using Root = void *;
}  // namespace lisp

/**
 * @class Lisp
 * @brief Utility class for working with Lisp objects in C++.
 * @ingroup uniot-lisp-helper
 *
 * Provides type definitions and helper methods for interacting with
 * and identifying Lisp object types in a type-safe manner.
 * @{
 */
class Lisp {
 public:
  /**
   * @enum Type
   * @brief Enumeration of supported Lisp data types.
   */
  enum Type : uint8_t {
    Unknown = 0, /**< Unknown or unsupported type */
    Int = 1,         /**< Integer value */
    Bool = 2,        /**< Boolean value (true/nil) */
    BoolInt = 3,     /**< Combined Boolean/Integer value */
    Symbol = 4,      /**< Symbolic reference */
    Cell = 5,        /**< List cell/pair */

    Any = 6          /**< Any valid type */
  };

  /**
   * @brief Checks if a type value is within the valid range.
   * @param type The type to validate.
   * @retval true Type is valid
   * @retval false Type is invalid
   */
  static inline bool correct(Lisp::Type type) {
    return (type >= Type::Int && type <= Lisp::Type::Any);
  }

  /**
   * @brief Converts a type enumeration value to a human-readable string.
   * @param type The type to convert to string.
   * @retval type String representation of the type
   */
  static inline const char *str(Lisp::Type type) {
    static const char *map[] = {
        "Unknown",
        "Int",
        "Bool",
        "Bool/Int",
        "Symbol",
        "Cell",

        "Any"};
    return correct(type) ? map[type] : map[Lisp::Type::Unknown];
  }

  /**
   * @brief Determines the Lisp::Type of a given Lisp object.
   * @param obj The Lisp object to analyze.
   * @retval type The corresponding Lisp::Type value.
   */
  static inline Lisp::Type getType(lisp::Object obj) {
    if (obj == nullptr) {
      return Lisp::Unknown;
    }

    switch (obj->type) {
      case TINT:
        return Lisp::Int;
      case TNIL:
        return Lisp::Bool;
      case TTRUE:
        return Lisp::Bool;
      case TSYMBOL:
        return Lisp::Symbol;
      case TCELL:
        return Lisp::Cell;
      default:
        return Lisp::Unknown;
    }
  }
};
/** @} */
}  // namespace uniot
