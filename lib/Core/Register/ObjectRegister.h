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
using RecordPtr = ObjectRegisterRecord *;

class ObjectRegister : public Register<Pair<uint32_t, RecordPtr>> {
 public:
  ObjectRegister() {}
  virtual ~ObjectRegister() {}

  ObjectRegister(ObjectRegister const &) = delete;
  void operator=(ObjectRegister const &) = delete;

  bool link(const String &name, RecordPtr link, uint32_t id = FOURCC(____)) {
    return addToRegister(name, MakePair(id, link));
  }

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

}  // namespace uniot
