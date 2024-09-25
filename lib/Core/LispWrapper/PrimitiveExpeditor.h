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

#pragma once

#include <Arduino.h>
#include <LispHelper.h>
#include <RegisterManagerProxy.h>
#include <setjmp.h>

namespace uniot {
using namespace lisp;

class PrimitiveExpeditor {
  class PrimitiveExpeditorInitializer;
  struct PrimitiveDescription {
    String name;
    uint8_t argsCount;
    Lisp::Type argsTypes[16];
    Lisp::Type returnType;
  };

 public:
  static RegisterManager &getRegisterManager() {
    return sRegister;
  }

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

  RegisterManagerProxy &getAssignedRegister() {
    return mRegisterProxy;
  }

  int getArgsLength() {
    return length(list());
  }

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

  Object evalList() {
    if (!mEvalList) {
      mEvalList = eval_list(mRoot, mEnv, mList);
    }
    return mEvalList;
  }

  Object list() {
    if (!mEvalList) {
      return *mList;
    }
    return mEvalList;
  }

  Object evalObj(VarObject obj) {
    return eval(mRoot, mEnv, obj);
  }

  void terminate(const char *msg) {
    error("[%s] %s", mDescription.name.c_str(), msg);
  }

  void assertArgsCount(int length) {
    if (getArgsLength() != length) {
      error_wrong_params_number();
    }
  }

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

  void assertDescribedArgs() {
    assertArgs(mDescription.argsCount, mDescription.argsTypes);
  }

  Object getArg(int idx) {
    const auto length = getArgsLength();
    if (idx >= length) {
      error("[%s] Trying to get %d arg out of %d", mDescription.name.c_str(), idx, length);
    }

    auto param = list();
    for (auto i = 0; i < idx; i++, param = param->cdr);

    return param->car;
  }

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

  const char *getArgSymbol(int idx) {
    auto arg = getArg(idx);

    if (!checkType(arg, Lisp::Symbol)) {
      error_invalid_type(idx, Lisp::Symbol, Lisp::getType(arg));
    }

    return arg->name;
  }

  Object makeBool(bool value) {
    return value ? True : Nil;
  }

  Object makeInt(int value) {
    return make_int(mRoot, value);
  }

  Object makeSymbol(const char *value) {
    return make_symbol(mRoot, value);
  }

 private:
  PrimitiveExpeditor(const PrimitiveDescription &description, Root root, VarObject env, VarObject list)
      : mDescription(description), mRoot(root), mEnv(env), mList(list), mEvalList(nullptr), mRegisterProxy(description.name, &sRegister) {
  }

  void error_invalid_type(int idx, Lisp::Type expectedType, Lisp::Type actualType) {
    error("[%s] Invalid type of %d parameter, expected <%s>, got <%s>", mDescription.name.c_str(), idx, Lisp::str(expectedType), Lisp::str(actualType));
  }

  void error_wrong_params_number() {
    error("[%s] Wrong number of params", mDescription.name.c_str());
  }

  PrimitiveDescription mDescription;
  Root mRoot;
  VarObject mEnv;
  VarObject mList;
  Object mEvalList;

  RegisterManagerProxy mRegisterProxy;
  static RegisterManager sRegister;

  static jmp_buf sDescriptionJumper;
  static PrimitiveDescription sLastDescription;

  class DescriptionModeGuard {
   public:
    DescriptionModeGuard() { sDescriptionMode = true; }
    ~DescriptionModeGuard() { sDescriptionMode = false; }
    static bool isDescriptionMode() { return sDescriptionMode; }

   private:
    static bool sDescriptionMode;
  };

  class PrimitiveExpeditorInitializer {
   public:
    PrimitiveExpeditor init(Root root, VarObject env, VarObject list) {
      return {PrimitiveExpeditor::sLastDescription, root, env, list};
    }
  };
};

RegisterManager PrimitiveExpeditor::sRegister;

jmp_buf PrimitiveExpeditor::sDescriptionJumper;
PrimitiveExpeditor::PrimitiveDescription PrimitiveExpeditor::sLastDescription;
bool PrimitiveExpeditor::DescriptionModeGuard::sDescriptionMode;
}  // namespace uniot
