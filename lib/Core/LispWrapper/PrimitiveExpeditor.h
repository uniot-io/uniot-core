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

/** @cond */
/**
 * DO NOT DELETE THE "uniot-lisp-primitives" GROUP DEFINITION BELOW.
 * Used to create the Utilities topic in the documentation. If you want to delete this file,
 * please paste the group definition into another file and delete this one.
 */
/** @endcond */

/**
 * @defgroup uniot-lisp-primitives Primitives
 * @ingroup uniot-lisp
 * @brief A collection of helper classes and functions for managing Lisp primitives.
 */

#pragma once

#include <Arduino.h>
#include <LispHelper.h>
#include <RegisterManagerProxy.h>
#include <setjmp.h>

namespace uniot {
using namespace lisp;

/**
 * @brief Manages and validates Lisp primitive operations.
 * @defgroup uniot-lisp-primitive-expeditor Expeditor
 * @ingroup uniot-lisp-primitives
 *
 * Provides utilities for describing lisp primitive functions, validating their arguments,
 * and extracting values from those arguments. It handles type checking and conversion
 * between Lisp types and C++ types.
 * @{
 */
class PrimitiveExpeditor {
  class PrimitiveExpeditorInitializer;
  /**
   * @struct PrimitiveDescription
   * @brief Stores metadata about a Lisp primitive function.
   */
  struct PrimitiveDescription {
    String name;         ///< Name of the primitive function
    uint8_t argsCount;   ///< Number of arguments the function expects
    Lisp::Type argsTypes[16]; ///< Types of each argument
    Lisp::Type returnType;    ///< Return type of the function
  };

 public:
  /**
   * @brief Gets the static register manager instance.
   * @retval RegisterManager& The register manager instance.
   */
  static RegisterManager &getRegisterManager() {
    return sRegister;
  }

  /**
   * @brief Describes a primitive function by setting up its metadata.
   *
   * @param name Name of the primitive function
   * @param returnType Return type of the function
   * @param argsCount Number of arguments the function expects
   * @param ... Types of each argument (variadic)
   * @retval initializer PrimitiveExpeditorInitializer object
   */
  static PrimitiveExpeditorInitializer describe(const String &name, Lisp::Type returnType, int argsCount, ...) {
    sLastDescription.name = name;
    sLastDescription.argsCount = argsCount;
    sLastDescription.returnType = returnType;
    memset(sLastDescription.argsTypes, 0, sizeof(sLastDescription.argsTypes));

    if (argsCount > 0) {
      va_list args;
      va_start(args, argsCount);

      for (auto i = 0; i < argsCount; i++) {
        auto type = va_arg(args, int);
        sLastDescription.argsTypes[i] = static_cast<Lisp::Type>(type);
      }
      va_end(args);
    }

    if (DescriptionModeGuard::isDescriptionMode()) {
      longjmp(sDescriptionJumper, 1);
    }
    return {};
  }

  /**
   * @brief Extracts description metadata from a primitive function.
   *
   * Uses setjmp/longjmp to capture the description information when the primitive
   * is executed in description mode.
   *
   * @param primitive Pointer to the primitive function
   * @retval description PrimitiveDescription object with the function's metadata
   */
  static PrimitiveDescription extractDescription(Primitive *primitive) {
    volatile auto guard = DescriptionModeGuard(); // volatile to prevent compiler from optimization
    if (primitive) {
      if (setjmp(sDescriptionJumper) == 0) {
        primitive(nullptr, nullptr, nullptr);
      } else {
        return sLastDescription;
      }
    }
    return {};
  }

  /**
   * @brief Gets the register proxy assigned to this expeditor.
   * @retval RegisterManagerProxy& The register manager proxy
   */
  RegisterManagerProxy &getAssignedRegister() {
    return mRegisterProxy;
  }

  /**
   * @brief Gets the number of arguments in the current function call.
   * @retval int The number of arguments in the list
   */
  int getArgsLength() {
    return length(list());
  }

  /**
   * @brief Checks if a parameter matches an expected Lisp type.
   *
   * @param param Object to check
   * @param expectedType Expected type of the object
   * @retval true Parameter matches the expected type
   * @retval false Parameter does not match the expected type
   */
  bool checkType(Object param, Lisp::Type expectedType) {
    auto paramType = param->type;
    switch (expectedType) {
      case Lisp::Int:
        if (paramType == TINT) {
          return true;
        }
        break;

      case Lisp::Bool:
        if (paramType == TNIL || paramType == TTRUE) {
          return true;
        }
        break;

      case Lisp::BoolInt:
        if (paramType == TINT || paramType == TNIL || paramType == TTRUE) {
          return true;
        }
        break;

      case Lisp::Symbol:
        if (paramType == TSYMBOL) {
          return true;
        }
        break;

      case Lisp::Cell:
        if (paramType == TCELL) {
          return true;
        }
        break;

      case Lisp::Any:
        return true;

      default:
        break;
    }
    return false;
  }

  /**
   * @brief Evaluates the argument list if not already evaluated.
   * @retval list The evaluated list of arguments
   */
  Object evalList() {
    if (!mEvalList) {
      mEvalList = eval_list(mRoot, mEnv, mList);
    }
    return mEvalList;
  }

  /**
   * @brief Gets the argument list (evaluated or not).
   * @retval list The list of arguments
   */
  Object list() {
    if (!mEvalList) {
      return *mList;
    }
    return mEvalList;
  }

  /**
   * @brief Evaluates a single object in the current environment.
   *
   * @param obj Object to evaluate
   * @retval Object The evaluated object
   */
  Object evalObj(VarObject obj) {
    return eval(mRoot, mEnv, obj);
  }

  /**
   * @brief Terminates execution with an error message.
   *
   * @param msg Error message to display
   */
  void terminate(const char *msg) {
    error("[%s] %s", mDescription.name.c_str(), msg);
  }

  /**
   * @brief Asserts that the function was called with the expected number of arguments.
   *
   * @param length Expected number of arguments
   * @throws Error if the argument count doesn't match
   */
  void assertArgsCount(int length) {
    if (getArgsLength() != length) {
      error_wrong_params_number();
    }
  }

  /**
   * @brief Asserts that arguments match the expected types.
   *
   * @param length Expected number of arguments
   * @param types Array of expected argument types
   * @throws Error if arguments don't match expected types or count
   */
  void assertArgs(uint8_t length, const Lisp::Type *types) {
    if (getArgsLength() != static_cast<int>(length)) {
      error_wrong_params_number();
    }

    auto param = evalList();
    for (uint8_t i = 0; i < length; i++, param = param->cdr) {
      if (!Lisp::correct(types[i])) {
        error("[%s] Type for %d parameter is not set", mDescription.name.c_str(), i);
      }
      if (param == Nil) {
        error("[%s] Unexpected end of params list at %d", mDescription.name.c_str(), i);
      }
      if (!checkType(param->car, types[i])) {
        error_invalid_type(i, types[i], Lisp::getType(param->car));
      }
    }
  }

  /**
   * @brief Variadic version of assertArgs.
   *
   * @param length Expected number of arguments
   * @param ... Expected argument types (variadic)
   * @throws Error if arguments don't match expected types or count
   */
  void assertArgs(uint8_t length, ...) {
    Lisp::Type types[length];

    va_list args;
    va_start(args, length);
    for (uint8_t i = 0; i < length; i++) {
      types[i] = static_cast<Lisp::Type>(va_arg(args, int));
    }
    va_end(args);

    assertArgs(length, types);
  }

  /**
   * @brief Asserts arguments against the primitive's description.
   * @throws Error if arguments don't match the description
   */
  void assertDescribedArgs() {
    assertArgs(mDescription.argsCount, mDescription.argsTypes);
  }

  /**
   * @brief Gets an argument at the specified index.
   *
   * @param idx Zero-based index of the argument to retrieve
   * @retval Object The argument object
   * @throws Error if index is out of bounds
   */
  Object getArg(int idx) {
    const auto length = getArgsLength();
    if (idx >= length) {
      error("[%s] Trying to get %d arg out of %d", mDescription.name.c_str(), idx, length);
    }

    auto param = list();
    for (auto i = 0; i < idx; i++, param = param->cdr);

    return param->car;
  }

  /**
   * @brief Gets a boolean value from an argument.
   *
   * @param idx Zero-based index of the argument
   * @param acceptsInt Whether to accept integer values as booleans
   * @retval bool The boolan value of the argument
   * @throws Error if argument doesn't have a compatible type
   */
  bool getArgBool(int idx, bool acceptsInt = true) {
    auto arg = getArg(idx);
    if (!mEvalList) {
      arg = evalObj(&arg);
    }

    auto type = acceptsInt ? Lisp::BoolInt : Lisp::Bool;
    if (!checkType(arg, type)) {
      error_invalid_type(idx, type, Lisp::getType(arg));
    }

    switch (arg->type) {
      case TINT:
        return arg->value != 0;
      case TTRUE:
        return true;
      case TNIL:
      default:
        return false;
    }
  }

  /**
   * @brief Gets an integer value from an argument.
   *
   * @param idx Zero-based index of the argument
   * @param acceptsBool Whether to accept boolean values as integers
   * @retval int The integer value of the argument
   * @throws Error if argument doesn't have a compatible type
   */
  int getArgInt(int idx, bool acceptsBool = true) {
    auto arg = getArg(idx);
    if (!mEvalList) {
      arg = evalObj(&arg);
    }

    auto type = acceptsBool ? Lisp::BoolInt : Lisp::Int;
    if (!checkType(arg, type)) {
      error_invalid_type(idx, type, Lisp::getType(arg));
    }

    switch (arg->type) {
      case TINT:
        return arg->value;
      case TTRUE:
        return 1;
      case TNIL:
      default:
        return 0;
    }
  }

  /**
   * @brief Gets a symbol value from an argument.
   *
   * @param idx Zero-based index of the argument
   * @retval symbol The symbol value of the argument
   * @throws Error if argument is not a symbol
   */
  const char *getArgSymbol(int idx) {
    auto arg = getArg(idx);

    if (!checkType(arg, Lisp::Symbol)) {
      error_invalid_type(idx, Lisp::Symbol, Lisp::getType(arg));
    }

    return arg->name;
  }

  /**
   * @brief Creates a boolean Lisp object.
   *
   * @param value Boolean value
   * @retval Object Lisp object representing the boolean value
   * @retval Nil If value is false
   */
  Object makeBool(bool value) {
    return value ? True : Nil;
  }

  /**
   * @brief Creates an integer Lisp object.
   *
   * @param value Integer value
   * @retval Object Lisp object representing the integer value
   */
  Object makeInt(int value) {
    return make_int(mRoot, value);
  }

  /**
   * @brief Creates a symbol Lisp object.
   *
   * @param value Symbol name
   * @retval Object Lisp object representing the symbol
   */
  Object makeSymbol(const char *value) {
    return make_symbol(mRoot, value);
  }

 private:
  /**
   * @brief Constructor for PrimitiveExpeditor.
   *
   * @param description Primitive function description
   * @param root Lisp root object
   * @param env Environment for evaluation
   * @param list List of arguments
   */
  PrimitiveExpeditor(const PrimitiveDescription &description, Root root, VarObject env, VarObject list)
      : mDescription(description), mRoot(root), mEnv(env), mList(list), mEvalList(nullptr), mRegisterProxy(description.name, &sRegister) {
  }

  /**
   * @brief Reports a type error for a parameter.
   *
   * @param idx Index of the parameter
   * @param expectedType Expected type
   * @param actualType Actual type received
   */
  void error_invalid_type(int idx, Lisp::Type expectedType, Lisp::Type actualType) {
    error("[%s] Invalid type of %d parameter, expected <%s>, got <%s>", mDescription.name.c_str(), idx, Lisp::str(expectedType), Lisp::str(actualType));
  }

  /**
   * @brief Reports an incorrect parameter count error.
   */
  void error_wrong_params_number() {
    error("[%s] Wrong number of params", mDescription.name.c_str());
  }

  PrimitiveDescription mDescription;  ///< Description of the primitive function
  Root mRoot;                         ///< Lisp root object
  VarObject mEnv;                     ///< Environment for evaluation
  VarObject mList;                    ///< List of arguments (not evaluated)
  Object mEvalList;                   ///< Evaluated list of arguments (lazily initialized)

  RegisterManagerProxy mRegisterProxy; ///< Proxy to the register manager
  static RegisterManager sRegister;    ///< Global register manager

  static jmp_buf sDescriptionJumper;             ///< Jump buffer for description extraction
  static PrimitiveDescription sLastDescription;  ///< Last description extracted

  /**
   * @class DescriptionModeGuard
   * @brief RAII guard for description mode.
   *
   * Controls the description mode flag, ensuring it is reset when going out of scope.
   */
  class DescriptionModeGuard {
   public:
    /**
     * @brief Constructor - sets description mode to true.
     */
    DescriptionModeGuard() { sDescriptionMode = true; }

    /**
     * @brief Destructor - sets description mode to false.
     */
    ~DescriptionModeGuard() { sDescriptionMode = false; }

    /**
     * @brief Checks if we're currently in description mode.
     * @retval true Description mode is active
     * @retval false Description mode is inactive
     */
    static bool isDescriptionMode() { return sDescriptionMode; }

   private:
    static bool sDescriptionMode;  ///< Flag indicating description mode
  };

  /**
   * @class PrimitiveExpeditorInitializer
   * @brief Initializer for PrimitiveExpeditor instances.
   *
   * Creates a PrimitiveExpeditor with the most recently described primitive.
   */
  class PrimitiveExpeditorInitializer {
   public:
    /**
     * @brief Initializes a PrimitiveExpeditor.
     *
     * @param root Lisp root object
     * @param env Environment for evaluation
     * @param list List of arguments
     * @retval expeditor The initialized expeditor
     */
    PrimitiveExpeditor init(Root root, VarObject env, VarObject list) {
      return {PrimitiveExpeditor::sLastDescription, root, env, list};
    }
  };
};

RegisterManager PrimitiveExpeditor::sRegister;

jmp_buf PrimitiveExpeditor::sDescriptionJumper;
PrimitiveExpeditor::PrimitiveDescription PrimitiveExpeditor::sLastDescription;
bool PrimitiveExpeditor::DescriptionModeGuard::sDescriptionMode;
/** @} */
}  // namespace uniot
