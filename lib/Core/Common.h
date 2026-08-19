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

#include <memory>
#include <utility>

/**
 * @defgroup common Common
 * @ingroup utils
 * @brief Common utilities and macros for the Uniot Core
 */

/**
 * @brief Hostname of the MQTT broker the device connects to.
 * @ingroup common
 */
#ifndef UNIOT_MQTT_HOST
#define UNIOT_MQTT_HOST "mqtt.uniot.io"
#endif

/**
 * @brief TCP port of the MQTT broker.
 * @ingroup common
 */
#ifndef UNIOT_MQTT_PORT
#define UNIOT_MQTT_PORT 1883
#endif

/**
 * @brief Prefix for the generated configuration AP SSID.
 * @ingroup common
 *
 * The full SSID is "PREFIX-DEVICEID", upper-cased.
 */
#ifndef UNIOT_WIFI_AP_PREFIX
#define UNIOT_WIFI_AP_PREFIX "UNIOT"
#endif

/**
 * @brief WPA2 password for the configuration AP.
 * @ingroup common
 *
 * An empty string creates an open network. WPA2 requires at least 8
 * characters; a shorter non-empty password makes softAP() fail and no
 * configuration AP is started.
 */
#ifndef UNIOT_WIFI_AP_PASSWORD
#define UNIOT_WIFI_AP_PASSWORD ""
#endif

/**
 * @brief Keep the WiFi radio awake between beacon intervals (ESP32 only).
 * @ingroup common
 *
 * Set to 1 to disable modem sleep once the station associates. This avoids
 * association drops caused by missed DTIM beacons on a marginal link, at the
 * cost of a significantly higher idle current. Leave at 0 on battery devices.
 */
#ifndef UNIOT_WIFI_NO_SLEEP
#define UNIOT_WIFI_NO_SLEEP 0
#endif

/**
 * @brief Creates a four-character code (FourCC) value from template parameters.
 * @ingroup common
 *
 * FourCC is commonly used for creating unique identifiers from four ASCII characters.
 * This template packs four 8-bit integers into a single 32-bit integer.
 *
 * @tparam a First character (least significant byte)
 * @tparam b Second character
 * @tparam c Third character
 * @tparam d Fourth character (most significant byte)
 */
template <int a, int b, int c, int d>
struct FourCC {
  static const uint32_t Value = (((((d << 8) | c) << 8) | b) << 8) | a;
};

/**
 * @brief Utility function to mark function parameters as intentionally unused.
 * @ingroup common
 *
 * Prevents compiler warnings about unused parameters by explicitly consuming them.
 *
 * @tparam Args Variadic template parameter types
 * @param args Parameters to mark as used
 */
template <typename... Args>
inline void UNUSED(Args &&...args) {
  (void)(sizeof...(args));
}

/**
 * @brief Calculates CRC32-C (Castagnoli) checksum for data integrity verification.
 * @ingroup common
 *
 * This function implements the CRC-32C polynomial used in iSCSI standard.
 *
 * @param data Pointer to data buffer
 * @param length Size of data buffer in bytes
 * @param crc Initial CRC value (defaults to 0)
 * @retval uint32_t CRC32 checksum value
 */
inline uint32_t CRC32(const void *data, size_t length, uint32_t crc = 0) {
  const uint8_t *ldata = (const uint8_t *)data;

  crc = ~crc;
  while (length--) {
    crc ^= *ldata++;
    for (uint8_t k = 0; k < 8; k++)
      crc = crc & 1 ? (crc >> 1) ^ 0x82f63b78 : crc >> 1;  // CRC-32C (iSCSI) polynomial in reversed bit order.
  }
  return ~crc;
}

/**
 * @brief Calculates the number of elements in a statically allocated array.
 * @ingroup common
 */
#define COUNT_OF(arr) (sizeof(arr) / sizeof(arr[0]))

/**
 * @brief Provides safe array access with bounds checking.
 * @ingroup common
 *
 * Returns the element at the specified index if within bounds,
 * otherwise returns the last element.
 */
#define ARRAY_ELEMENT_SAFE(arr, index) ((arr)[(((index) < COUNT_OF(arr)) ? (index) : (COUNT_OF(arr) - 1))])

/**
 * @brief Creates a FourCC constant from a string literal.
 * @ingroup common
 *
 * Uses up to the first four characters of the string to create a unique identifier.
 * If the string has fewer than 4 characters, the remaining positions will be filled with
 * the last character.
 */
#define FOURCC(name) FourCC<ARRAY_ELEMENT_SAFE(#name, 0), ARRAY_ELEMENT_SAFE(#name, 1), ARRAY_ELEMENT_SAFE(#name, 2), ARRAY_ELEMENT_SAFE(#name, 3)>::Value

/**
 * @brief Creates an alias for a function that forwards all arguments implicitly.
 * @ingroup common
 *
 * Used to expose standard library functions through a custom namespace with the
 * same parameter forwarding behavior.
 *
 * @param high New function name
 * @param low Original function being aliased
 */
#define ALIAS_IMPLICIT_FUNCTION(high, low)                                         \
  template <typename... Args>                                                      \
  inline auto high(Args &&...args) -> decltype(low(std::forward<Args>(args)...)) { \
    return low(std::forward<Args>(args)...);                                       \
  }

/**
 * @brief Creates an alias for a template function that forwards all arguments explicitly.
 * @ingroup common
 *
 * Similar to ALIAS_IMPLICIT_FUNCTION but maintains the explicit template parameter.
 *
 * @param high New function name
 * @param low Original function being aliased
 */
#define ALIAS_EXPLICIT_FUNCTION(high, low)                                            \
  template <typename T, typename... Args>                                             \
  inline auto high(Args &&...args) -> decltype(low<T>(std::forward<Args>(args)...)) { \
    return low<T>(std::forward<Args>(args)...);                                       \
  }

namespace uniot {
/**
 * @ingroup common
 * @{
 */

/**
 * @brief Type alias for std::unique_ptr with cleaner syntax.
 *
 * @tparam T Type of the managed object
 */
template <typename T>
using UniquePointer = std::unique_ptr<T>;

/**
 * @brief Type alias for std::shared_ptr with cleaner syntax.
 *
 * @tparam T Type of the managed object
 */
template <typename T>
using SharedPointer = std::shared_ptr<T>;

/**
 * @brief Type alias for std::pair with cleaner syntax.
 *
 * @tparam T_First Type of the first element
 * @tparam T_Second Type of the second element
 */
template <typename T_First, typename T_Second>
using Pair = std::pair<T_First, T_Second>;

/**
 * @brief Creates a shared pointer instance, alias for std::make_shared.
 */
ALIAS_EXPLICIT_FUNCTION(MakeShared, std::make_shared)

/**
 * @brief Creates a unique pointer instance, alias for std::make_unique.
 */
ALIAS_EXPLICIT_FUNCTION(MakeUnique, std::make_unique)

/**
 * @brief Creates a pair instance, alias for std::make_pair.
 */
ALIAS_IMPLICIT_FUNCTION(MakePair, std::make_pair)

/** @} */
}  // namespace uniot
