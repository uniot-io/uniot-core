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

#include <functional>
#include <unLisp.h>
#include <Arduino.h>

namespace uniot
{
enum LispType
{
  Int,
  Bool,
  BoolInt,
  Symbol
};

class UserPrimitive
{
public:
  UserPrimitive(const String &name, Root root, VarObject env, VarObject list)
      : mName(name), mRoot(root), mEnv(env), mList(list), mEvalList(nullptr)
  {
  }

  int getArgsLength()
  {
    return length(*mList);
  }

  bool checkType(Object param, LispType type)
  {
    auto paramType = param->car->type;
    switch (type)
    {
    case Int:
      if (paramType == TINT)
        return true;
      break;

    case Bool:
      if (paramType == TNIL || paramType == TTRUE)
        return true;
      break;

    case BoolInt:
      if (paramType == TINT || paramType == TNIL || paramType == TTRUE)
        return true;
      break;

    case Symbol:
      if (paramType == TSYMBOL)
        return true;
      break;

    default:
      break;
    }
    return false;
  }

  Object evalList() {
    if (!mEvalList)
      mEvalList = eval_list(mRoot, mEnv, mList);
    return mEvalList;
  }

  void assertArgs(int length, ...)
  {
    if (getArgsLength() != length)
      error("[%s] Wrong number of params", mName.c_str());

    va_list args;
    va_start(args, length);

    auto param = evalList();
    for (auto i = 0; i < length; i++, param = param->cdr)
    {
      auto type = va_arg(args, int);

      if (param == Nil)
        error("[%s] Unexpected end of params list at %d", mName.c_str(), i + 1);
      if (!checkType(param, static_cast<LispType>(type)))
        error("[%s] Invalid type of %d parameter", mName.c_str(), i + 1);
    }
    va_end(args);
  }

private:
  String mName;
  Root mRoot;
  VarObject mEnv;
  VarObject mList;
  Object mEvalList;
};

} // namespace uniot
