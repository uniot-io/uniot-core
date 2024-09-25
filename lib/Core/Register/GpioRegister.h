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
#include <DefaultPrimitives.h>

#include "Register.h"

namespace uniot {

/**
 * @brief Specialized GpioRegister class for GPIO pin management.
 *
 * Inherits from the generalized Register class to handle GPIO-specific functionalities.
 */
class GpioRegister : public Register<uint8_t> {
 public:
  GpioRegister() : Register<uint8_t>() {}
  ~GpioRegister() {}

  GpioRegister(GpioRegister const&) = delete;
  void operator=(GpioRegister const&) = delete;

  template <typename... Args>
  void setDigitalInput(uint8_t first, Args... args) {
    setRegisterVariadic(primitive::name::dread, first, args...);
  }

  template <typename... Args>
  void setDigitalOutput(uint8_t first, Args... args) {
    setRegisterVariadic(primitive::name::dwrite, first, args...);
  }

  template <typename... Args>
  void setAnalogInput(uint8_t first, Args... args) {
    setRegisterVariadic(primitive::name::aread, first, args...);
  }

  template <typename... Args>
  void setAnalogOutput(uint8_t first, Args... args) {
    setRegisterVariadic(primitive::name::awrite, first, args...);
  }

 protected:
  virtual void _processRegister(const String& name, const uint8_t& value) override {
    if (name == primitive::name::dread) {
      pinMode(value, INPUT);
    } else if (name == primitive::name::dwrite) {
      pinMode(value, OUTPUT);
    } else if (name == primitive::name::aread) {
      pinMode(value, INPUT);
    } else if (name == primitive::name::awrite) {
      pinMode(value, OUTPUT);
    }
  }

 private:
  template <typename... Args>
  void setRegisterVariadic(const String& name, uint8_t first, Args... args) {
    uint8_t pins[] = {first, static_cast<uint8_t>(args)...};
    size_t count = sizeof...(args) + 1;
    setRegister(name, pins, count);
  }
};

}  // namespace uniot
