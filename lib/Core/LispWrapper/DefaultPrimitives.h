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

/**
 * @file DefaultPrimitives.h
 * @brief Defines string constants for Lisp primitive function names.
 *
 * This header provides standardized names for primitive operations that can be used
 * in Lisp expressions for interacting with hardware I/O and device functionality.
 */

#pragma once

/**
 * @namespace uniot::primitive::name
 * @brief Contains string constants for Lisp primitive function names.
 */
namespace uniot::primitive::name {

/**
 * @namespace uniot::primitive::name
 * @defgroup uniot-lisp-primitives-default Default Primitives
 * @ingroup uniot-lisp-primitives
 * @brief Contains string constants representing names of primitive functions available in the Lisp environment.
 *
 * These constants define the standard function names that can be called from Lisp code
 * to interact with the hardware and peripherals of Uniot devices.
 * @{
 */

/**
 * @brief Primitive for digital write operations.
 *
 * Used to write a digital value (HIGH/LOW) to a specified digital pin.
 * Typical usage: (dwrite pin-number value)
 */
constexpr const char* dwrite = "dwrite";

/**
 * @brief Primitive for digital read operations.
 *
 * Used to read the current digital value (HIGH/LOW) from a specified digital pin.
 */
constexpr const char* dread = "dread";

/**
 * @brief Primitive for analog write operations.
 *
 * Used to write an analog value (PWM) to a supported pin.
 */
constexpr const char* awrite = "awrite";

/**
 * @brief Primitive for analog read operations.
 *
 * Used to read an analog value from a specified analog input pin.
 */
constexpr const char* aread = "aread";

/**
 * @brief Primitive for detecting button click events.
 *
 * Used to determine if a button connected to the specified pin has been clicked.
 */
constexpr const char* bclicked = "bclicked";
/** @} */

}  // namespace uniot::primitive::name
