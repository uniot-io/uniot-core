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

#include "CBORObject.h"
#include "GpioRegister.h"
#include "ObjectRegister.h"

namespace uniot {

class RegisterManager {
 public:
  RegisterManager() {}
  RegisterManager(RegisterManager const &) = delete;
  void operator=(RegisterManager const &) = delete;

  template <typename... Args>
  void setDigitalInput(uint8_t first, Args... args) {
    mGpioRegistry.setDigitalInput(first, args...);
  }

  template <typename... Args>
  void setDigitalOutput(uint8_t first, Args... args) {
    mGpioRegistry.setDigitalOutput(first, args...);
  }

  template <typename... Args>
  void setAnalogInput(uint8_t first, Args... args) {
    mGpioRegistry.setAnalogInput(first, args...);
  }

  template <typename... Args>
  void setAnalogOutput(uint8_t first, Args... args) {
    mGpioRegistry.setAnalogOutput(first, args...);
  }

  bool link(const String &name, RecordPtr link, uint32_t id = FOURCC(____)) {
    return mObjectRegistry.link(name, link, id);
  }

  bool getGpio(const String &name, size_t index, uint8_t &outValue) const {
    return mGpioRegistry.getRegisterValue(name, index, outValue);
  }

  template <typename T>
  T *getObject(const String &name, size_t index) {
    return mObjectRegistry.get<T>(name, index);
  }

  void serializeRegisters(CBORObject &obj) {
    mGpioRegistry.iterateRegisters([&](const String &name, SharedPointer<Array<uint8_t>> values) {
      obj.putArray(name.c_str()).append(values->size(), values->raw());
    });
    mObjectRegistry.iterateRegisters([&](const String &name, SharedPointer<Array<Pair<uint32_t, RecordPtr>>> values) {
      auto arr = obj.putArray(name.c_str());
      for (size_t i = 0; i < values->size(); i++) {
        auto &pair = (*values)[i];
        arr.append(pair.first);
      }
    });
  }

 private:
  ObjectRegister mObjectRegistry;
  GpioRegister mGpioRegistry;
};

}  // namespace uniot
