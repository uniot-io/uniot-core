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
 * @brief Packs a semantic version into one integer.
 * @ingroup common
 *
 * Uses the same formula as uniot-lisp's SEMVER_TO_INT, so a core version and an interpreter
 * version are comparable and read the same way. It carries its own name so that including
 * both headers cannot make one definition silently stand for the other.
 */
#define UNIOT_SEMVER_TO_INT(major, minor, patch) ((major * 10000) + (minor * 100) + patch)

/**
 * @brief Version of Uniot Core, reported in the device's status packet.
 * @ingroup common
 *
 * Neither this nor UNIOT_SEMVER_TO_INT is guarded, so a sketch cannot redefine them: they
 * describe the core that is running, not something a sketch chooses. Keep this in step with
 * the version in library.json.
 */
#define UNIOT_CORE_VERSION UNIOT_SEMVER_TO_INT(0, 8, 1)

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
 * @brief Consecutive reboots that reset the stored network configuration.
 * @ingroup common
 *
 * Each reboot within UNIOT_WIFI_REBOOT_WINDOW_MS of the last one increments a
 * counter held in ctrl.cbor; reaching this many clears the credentials. It is
 * the only reset path on a device with no button, so it is a deliberate gesture
 * -- power-cycling this many times in quick succession -- and the count trades
 * ease of use against wiping a device by accident on flaky power.
 *
 * This is the count used once the mechanism is switched on, not whether it is on.
 * UniotCore leaves reboot-reset disabled until configWiFiResetOnReboot() asks for
 * it, precisely because of that accidental-wipe risk; this value is what that call
 * uses when given no count of its own.
 */
#ifndef UNIOT_WIFI_REBOOT_RESET_COUNT
#define UNIOT_WIFI_REBOOT_RESET_COUNT 5
#endif

/**
 * @brief How long after boot a reboot still counts towards the reset, in ms.
 * @ingroup common
 *
 * Staying up longer than this clears the counter, so ordinary restarts do not
 * accumulate towards a reset. Ignored when the count is UINT8_MAX.
 */
#ifndef UNIOT_WIFI_REBOOT_WINDOW_MS
#define UNIOT_WIFI_REBOOT_WINDOW_MS 10000
#endif

/**
 * @brief Bytes of heap the Lisp interpreter is given at startup.
 * @ingroup common
 *
 * Allocated once in lisp_create() and never grown, so it is a fixed claim on system heap
 * for the life of the machine. Creating a machine costs ~3150 bytes before a script runs,
 * so what a script actually has is this value less that. The library refuses anything
 * above 65535.
 */
#ifndef UNIOT_LISP_HEAP
// Not split by ESP32 variant: object sizes are identical on both parts.
#if defined(ESP32)
#define UNIOT_LISP_HEAP 24576
#elif defined(ESP8266)
#define UNIOT_LISP_HEAP 12288
#else
#define UNIOT_LISP_HEAP 8192
#endif
#endif

/**
 * @brief Bytes of C stack an evaluation may spend before a script is abandoned.
 * @ingroup common
 *
 * Interpreter recursion runs on the C stack, and overrunning it resets the device rather
 * than raising an error. The budget is in bytes, not levels of nesting, since a level that
 * prints costs several times what a bare call costs.
 *
 * It is not simply the stack that is free. The guard measures from where lisp_eval() was
 * entered and stops seeing anything once a primitive or the error path takes over, so the
 * true peak is the budget plus roughly 600 bytes paid afterwards, during unwinding. Each
 * value below is measured on target against the full firmware, not the standalone tool,
 * with the probe in src/main.cpp; measure a budget before changing it rather than scaling
 * it from another part.
 *
 * Keep the ESP32 branches separate: defined(ESP32) is true for the C3, so collapsing them
 * means a retune of one silently retunes the other.
 */
#ifndef UNIOT_LISP_MAX_EVAL_STACK
#if defined(ESP8266)
#define UNIOT_LISP_MAX_EVAL_STACK 1280
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
#define UNIOT_LISP_MAX_EVAL_STACK 3072
#elif defined(ESP32)
#define UNIOT_LISP_MAX_EVAL_STACK 3072
#else
// An unknown platform is assumed to be a small MCU, not a host: too little heap raises a
// clean error, too much stack resets the device, so the fallbacks lean small. A host build
// with room to spare should set both explicitly rather than inherit these.
#define UNIOT_LISP_MAX_EVAL_STACK 2048
#endif
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
