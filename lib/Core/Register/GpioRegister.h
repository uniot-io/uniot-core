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
 * @defgroup registers_gpio_register GPIO Register
 * @ingroup registers
 * @{
 *
 * Inherits from the generalized Register class to handle GPIO-specific functionalities.
 * This class provides a convenient interface for configuring GPIO pins for different modes
 * including digital input/output and analog input/output.
 */
class GpioRegister : public Register<uint8_t> {
 public:
  /**
   * @brief Default constructor.
   *
   * Initializes a new instance of the GpioRegister class.
   */
  GpioRegister() : Register<uint8_t>() {}

  /**
   * @brief Destructor.
   */
  ~GpioRegister() {}

  /**
   * @brief Deleted copy constructor to prevent copying.
   */
  GpioRegister(GpioRegister const&) = delete;

  /**
   * @brief Deleted assignment operator to prevent assignment.
   */
  void operator=(GpioRegister const&) = delete;

  /**
   * @brief Configure pins as digital inputs.
   *
   * Sets up one or more pins as digital inputs using Arduino's INPUT mode.
   *
   * @tparam Args Variadic template for accepting multiple pin numbers.
   * @param first The first pin number to configure.
   * @param args Additional pin numbers to configure.
   */
  template <typename... Args>
  void setDigitalInput(uint8_t first, Args... args) {
    setRegisterVariadic(primitive::name::dread, first, args...);
  }

  /**
   * @brief Configure pins as digital outputs.
   *
   * Sets up one or more pins as digital outputs using Arduino's OUTPUT mode.
   *
   * @tparam Args Variadic template for accepting multiple pin numbers.
   * @param first The first pin number to configure.
   * @param args Additional pin numbers to configure.
   */
  template <typename... Args>
  void setDigitalOutput(uint8_t first, Args... args) {
    setRegisterVariadic(primitive::name::dwrite, first, args...);
  }

  /**
   * @brief Configure pins as analog inputs.
   *
   * Sets up one or more pins as analog inputs using Arduino's INPUT mode.
   * These pins typically connect to ADC channels.
   *
   * @tparam Args Variadic template for accepting multiple pin numbers.
   * @param first The first pin number to configure.
   * @param args Additional pin numbers to configure.
   */
  template <typename... Args>
  void setAnalogInput(uint8_t first, Args... args) {
    setRegisterVariadic(primitive::name::aread, first, args...);
  }

  /**
   * @brief Configure pins as analog outputs.
   *
   * Sets up one or more pins as analog outputs using Arduino's OUTPUT mode.
   * These pins typically connect to PWM or DAC channels.
   *
   * @tparam Args Variadic template for accepting multiple pin numbers.
   * @param first The first pin number to configure.
   * @param args Additional pin numbers to configure.
   */
  template <typename... Args>
  void setAnalogOutput(uint8_t first, Args... args) {
    setRegisterVariadic(primitive::name::awrite, first, args...);
  }

 protected:
  /**
   * @brief Override method to process register values.
   *
   * Configures the pin mode based on the register name:
   * - dread: configures for digital reading (INPUT)
   * - dwrite: configures for digital writing (OUTPUT)
   * - aread: configures for analog reading (INPUT)
   * - awrite: configures for analog writing (OUTPUT)
   *
   * @param name The register name (dread, dwrite, aread, or awrite).
   * @param value The pin number to configure.
   */
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
  /**
   * @brief Helper method to process multiple pin registrations.
   *
   * Converts variadic arguments to an array and passes them to setRegister.
   * This allows convenient registration of multiple pins with the same mode.
   *
   * @tparam Args Variadic template for accepting multiple pin numbers.
   * @param name The register name to associate with the pins.
   * @param first The first pin number to register.
   * @param args Additional pin numbers to register.
   */
  template <typename... Args>
  void setRegisterVariadic(const String& name, uint8_t first, Args... args) {
    uint8_t pins[] = {first, static_cast<uint8_t>(args)...};
    size_t count = sizeof...(args) + 1;
    setRegister(name, pins, count);
  }
};
/** @} */
}  // namespace uniot
