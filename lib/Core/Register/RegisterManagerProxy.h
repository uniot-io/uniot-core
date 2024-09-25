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

class RegisterManagerProxy {
 public:
  RegisterManagerProxy(const RegisterManagerProxy &) = delete;
  void operator=(const RegisterManagerProxy &) = delete;

  RegisterManagerProxy(const String &name, RegisterManager *reg)
      : mName(name), mpRegister(reg) {
  }

  bool getGpio(size_t index, uint8_t &outValue) const {
    return mpRegister->getGpio(mName, index, outValue);
  }

  template <typename T>
  T *getObject(size_t index) {
    return mpRegister->getObject<T>(mName, index);
  }

 private:
  String mName;
  RegisterManager *mpRegister;
};

}  // namespace uniot
