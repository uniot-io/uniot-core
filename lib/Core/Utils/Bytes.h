/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2023 Uniot <contact@uniot.io>
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

#include <Common.h>
#include <Logger.h>
#include <WString.h>
#include <stdio.h>

#include <functional>
#include <type_traits>

/**
 * @brief A class for managing and manipulating byte arrays.
 * @defgroup utils_bytes Bytes
 * @ingroup utils
 * @{
 *
 * The Bytes class provides a convenient wrapper around raw byte arrays with
 * automatic memory management, string conversions, and utility functions.
 */
class Bytes {
 public:
  /**
   * @brief Function type for filling byte arrays.
   *
   * A callable that takes a buffer and size, fills the buffer, and returns the number of bytes filled.
   */
  using Filler = std::function<size_t(uint8_t *buf, size_t size)>;

  /**
   * @brief Default constructor.
   *
   * Creates an empty Bytes object with no allocated memory.
   */
  Bytes() : Bytes(nullptr, 0) {}

  /**
   * @brief Constructor from raw byte array.
   *
   * If data is nullptr but size is non-zero, space will be allocated but not initialized.
   *
   * @param data Pointer to byte array to copy, or nullptr to just allocate
   * @param size Size of the byte array
   */
  Bytes(const uint8_t *data, size_t size) {
    _init();
    if (data) {
      _copy(data, size);
    } else if (size) {
      _reserve(size);
    }
  }

  /**
   * @brief Constructor from C-string.
   *
   * Creates a Bytes object containing the string plus its null terminator.
   *
   * @param str Null-terminated C string
   */
  Bytes(const char *str) : Bytes((uint8_t *)str, strlen(str) + 1)  // + 1 null terminator
  {}

  /**
   * @brief Constructor from integral type.
   *
   * @param value Integral value to convert to bytes
   *
   * Creates a Bytes object with the binary representation of the integral value.
   * Only works with integral types (int, char, long, etc.).
   */
  template <typename T>
  Bytes(T value) : Bytes(((uint8_t *)&value), sizeof(T)) {
    static_assert(std::is_integral<T>::value, "only integral types are allowed");
  }

  /**
   * @brief Copy constructor.
   *
   * Creates a deep copy of another Bytes object.
   *
   * @param value The Bytes object to copy
   */
  Bytes(const Bytes &value) {
    _init();
    *this = value;
  }

  /**
   * @brief Constructor from Arduino String.
   *
   * Creates a Bytes object from the contents of an Arduino String.
   *
   * @param value Arduino String to convert to bytes
   */
  Bytes(const String &value) {
    _init();
    *this = value;
  }

  /**
   * @brief Destructor.
   *
   * Frees allocated memory.
   */
  virtual ~Bytes() {
    _invalidate();
  }

  /**
   * @brief Assignment operator.
   *
   * Creates a deep copy of the data from another Bytes object.
   *
   * @param rhs The Bytes object to copy from
   * @retval Bytes& Reference to this object
   */
  Bytes &operator=(const Bytes &rhs) {
    if (this != &rhs) {
      if (rhs.mBuffer) {
        _copy(rhs.mBuffer, rhs.mSize);
      } else {
        _invalidate();
      }
    }
    return *this;
  }

  /**
   * @brief Assignment from Arduino String.
   *
   * Copies the contents of an Arduino String and adds a null terminator.
   *
   * @param rhs Arduino String to copy from
   * @retval Bytes& Reference to this object
   */
  Bytes &operator=(const String &rhs) {
    if (rhs.length()) {
      _copy((const uint8_t *)rhs.c_str(), rhs.length());
    } else {
      _invalidate();
    }
    return this->terminate();
  }

  // template <typename T>
  // operator T() {
  //   return *((T *)mBuffer);
  // }

  /**
   * @brief Creates a Bytes object from a hexadecimal string.
   *
   * Converts a hexadecimal string to its binary representation.
   * The hexadecimal string length must be even.
   *
   * @param hexStr String containing hexadecimal characters (e.g. "1A2B3C")
   * @retval Bytes Object with the binary data
   * @retval Bytes Empty object if the string length is invalid
   */
  static Bytes fromHexString(const String &hexStr) {
    size_t len = hexStr.length();
    if (len % 2 != 0) {
      UNIOT_LOG_ERROR("invalid hex string length");
      return Bytes();
    }

    Bytes bytes;
    bytes._reserve(len / 2);
    for (size_t i = 0; i < len; i += 2) {
      char buf[3] = {hexStr.charAt(i), hexStr.charAt(i + 1), '\0'};
      uint8_t b = strtol(buf, nullptr, 16);
      bytes.mBuffer[i / 2] = b;
    }
    return bytes;
  }

  /**
   * @brief Fills the buffer using a provided filler function.
   *
   * Uses the provided function to fill the buffer with data.
   * The filler should return the number of bytes actually written.
   *
   * @param filler Function to fill the buffer
   * @retval size_t Number of bytes filled
   * @retval 0 If the filler function is null
   */
  size_t fill(Filler filler) {
    if (filler) {
      return filler(mBuffer, mSize);
    }
    return 0;
  }

  /**
   * @brief Reduces the size of the buffer.
   *
   * Reduces the size of the buffer. If newSize is larger than the current size,
   * no action is taken.
   *
   * @param newSize The new size to shrink to
   * @retval Bytes& Reference to this object
   */
  Bytes &prune(size_t newSize) {
    if (newSize < mSize) {
      _reserve(newSize);
    }
    return *this;
  }

  /**
   * @brief Gets a const pointer to the raw byte array.
   *
   * Provides direct access to the internal buffer. Do not modify or free this pointer.
   *
   * @retval uint8_t Pointer to this object
   */
  const uint8_t *raw() const {
    return mBuffer;
  }

  /**
   * @brief Ensures the byte array is null-terminated.
   *
   * Adds a null terminator if one isn't already present, useful for string operations.
   *
   * @retval Bytes& Reference to this object
   */
  Bytes &terminate() {
    if (!mSize) {
      _reserve(1);
      return *this;
    }

    if (mBuffer[mSize - 1] != '\0') {
      _reserve(mSize + 1);
      mBuffer[mSize - 1] = '\0';
    }
    return *this;
  }

  /**
   * @brief Gets the byte array as a C string.
   *
   * Returns the buffer as a C string. The buffer should be null-terminated first.
   *
   * @retval char* Pointer to the internal buffer
   */
  const char *c_str() const {
    return (const char *)mBuffer;
  }

  /**
   * @brief Converts the byte array to an Arduino String.
   *
   * Assumes the bytes represent a null-terminated string.
   *
   * @retval String String representation of the bytes as a character string
   */
  String toString() const {
    return String(this->c_str());
  }

  /**
   * @brief Converts the byte array to a hexadecimal string.
   *
   * Each byte is converted to a two-character hexadecimal representation.
   *
   * @retval String String containing hexadecimal representation of the bytes
   */
  String toHexString() const {
    String result = "";
    for (size_t i = 0; i < mSize; i++) {
      char buf[3];
      snprintf(buf, sizeof(buf), "%02X", mBuffer[i]);
      result += buf;
    }
    return result;
  }

  /**
   * @brief Gets the size of the byte array.
   *
   * @retval size_t Size of the byte array in bytes
   */
  size_t size() const {
    return mSize;
  }

  /**
   * @brief Deallocates the internal buffer.
   *
   * Frees all allocated memory and resets the object to empty state.
   */
  void clean() {
    _invalidate();
  }

  /**
   * @brief Calculates a checksum of the byte array.
   *
   * Uses CRC32 algorithm to generate a checksum of the data.
   *
   * @retval uint32_t Checksum of the byte array
   */
  uint32_t checksum() const {
    return CRC32(mBuffer, mSize);
  }

 private:
  /**
   * @brief Initializes object to empty state.
   *
   * Sets buffer to nullptr and size to 0.
   */
  inline void _init(void) {
    mBuffer = nullptr;
    mSize = 0;
  }

  /**
   * @brief Frees memory and resets to empty state.
   *
   * Deallocates any memory and initializes to empty.
   */
  void _invalidate(void) {
    if (mBuffer) {
      free(mBuffer);
    }
    _init();
  }

  /**
   * @brief Allocates or reallocates memory for the buffer.
   *
   * Allocates memory of the specified size. If the new size is larger,
   * the additional memory is zero-initialized.
   *
   * @param newSize Size to allocate
   * @retval true Memory was successfully allocated
   * @retval false Memory allocation failed
   */
  bool _reserve(size_t newSize) {
    mBuffer = (uint8_t *)realloc(mBuffer, newSize);
    if (mBuffer && (newSize > mSize)) {
      memset(mBuffer + mSize, 0, newSize - mSize);
    }
    mSize = newSize;
    return mBuffer;
  }

  /**
   * @brief Copies data from a source buffer.
   *
   * Allocates memory and copies the data into this object.
   *
   * @param data Source data to copy
   * @param size Size of the data
   * @retval Bytes& Reference to this object
   */
  Bytes &_copy(const uint8_t *data, size_t size) {
    if (_reserve(size)) {
      memcpy(mBuffer, data, size);
    } else {
      _invalidate();
    }
    return *this;
  }

  uint8_t *mBuffer;  ///< Pointer to the dynamically allocated buffer
  size_t mSize;      ///< Size of the allocated buffer in bytes
};
/** @} */
