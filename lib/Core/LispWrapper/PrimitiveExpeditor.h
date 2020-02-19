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
#include <LinksRegisterProxy.h>

namespace uniot
{
using namespace lisp;

class PrimitiveExpeditor
{
public:
  PrimitiveExpeditor(const String &name, Root root, VarObject env, VarObject list)
      : mName(name), mRoot(root), mEnv(env), mList(list),
        mEvalList(nullptr),
        mRegisterProxy(name, &sRegister)
  {
  }

  static LinksRegister &getGlobalRegister()
  {
    return sRegister;
  }

  LinksRegisterProxy &getCurrentRegister()
  {
    return mRegisterProxy;
  }

  int getArgsLength()
  {
    return length(list());
  }

  bool checkType(Object param, Lisp::Type type)
  {
    auto paramType = param->type;
    switch (type)
    {
    case Lisp::Int:
      if (paramType == TINT)
        return true;
      break;

    case Lisp::Bool:
      if (paramType == TNIL || paramType == TTRUE)
        return true;
      break;

    case Lisp::BoolInt:
      if (paramType == TINT || paramType == TNIL || paramType == TTRUE)
        return true;
      break;

    case Lisp::Symbol:
      if (paramType == TSYMBOL)
        return true;
      break;

    case Lisp::Cell:
      if (paramType == TCELL)
        return true;
      break;

    case Lisp::Any:
      return true;

    default:
      break;
    }
    return false;
  }

  Object evalList()
  {
    if (!mEvalList)
      mEvalList = eval_list(mRoot, mEnv, mList);
    return mEvalList;
  }

  Object list()
  {
    if (!mEvalList)
      return *mList;
    return mEvalList;
  }

  Object evalObj(VarObject obj)
  {
    return eval(mRoot, mEnv, obj);
  }

  void terminate(const char *msg)
  {
    error("[%s] %s", mName.c_str(), msg);
  }

  void assertArgsCount(int length)
  {
    if (getArgsLength() != length)
      error_wrong_params_number();
  }

  void assertArgs(int length, ...)
  {
    if (getArgsLength() != length)
      error_wrong_params_number();

    if (length <= 0)
      return;

    va_list args;
    va_start(args, length);

    auto param = evalList();
    for (auto i = 0; i < length; i++, param = param->cdr)
    {
      auto type = va_arg(args, int);

      if (!Lisp::correct(static_cast<Lisp::Type>(type)))
        error("[%s] Type for %d parameter is not set", mName.c_str(), i);
      if (param == Nil)
        error("[%s] Unexpected end of params list at %d", mName.c_str(), i);
      if (!checkType(param->car, static_cast<Lisp::Type>(type)))
        error_invalid_type(i, static_cast<Lisp::Type>(type));
    }
    va_end(args);
  }

  Object getArg(int idx)
  {
    const auto length = getArgsLength();
    if (idx >= length)
      error("[%s] Trying to get %d arg out of %d", mName.c_str(), idx, length);

    auto param = list();
    for (auto i = 0; i < idx; i++, param = param->cdr)
      ;

    return param->car;
  }

  bool getArgBool(int idx, bool acceptsInt = true)
  {
    auto arg = getArg(idx);
    if (!mEvalList)
      arg = evalObj(&arg);

    auto type = acceptsInt ? Lisp::BoolInt : Lisp::Bool;
    if (!checkType(arg, type))
      error_invalid_type(idx, type);

    switch (arg->type)
    {
    case TINT:
      return arg->value != 0;
    case TTRUE:
      return true;
    case TNIL:
    default:
      return false;
    }
  }

  int getArgInt(int idx)
  {
    auto arg = getArg(idx);
    if (!mEvalList)
      arg = evalObj(&arg);

    if (!checkType(arg, Lisp::Int))
      error_invalid_type(idx, Lisp::Int);

    return arg->value;
  }

  const char *getArgSymbol(int idx)
  {
    auto arg = getArg(idx);

    if (!checkType(arg, Lisp::Symbol))
      error_invalid_type(idx, Lisp::Symbol);

    return arg->name;
  }

  Object makeBool(bool value)
  {
    return value ? True : Nil;
  }

  Object makeInt(int value)
  {
    return make_int(mRoot, value);
  }

  Object makeSymbol(const char *value)
  {
    return make_symbol(mRoot, value);
  }

private:
  void error_invalid_type(int idx, Lisp::Type type)
  {
    error("[%s] Invalid type of %d parameter, expected <%s>", mName.c_str(), idx, Lisp::str(type));
  }

  void error_wrong_params_number()
  {
    error("[%s] Wrong number of params", mName.c_str());
  }

  String mName;
  Root mRoot;
  VarObject mEnv;
  VarObject mList;
  Object mEvalList;

  LinksRegisterProxy mRegisterProxy;
  static LinksRegister sRegister;
};

LinksRegister PrimitiveExpeditor::sRegister;

} // namespace uniot
