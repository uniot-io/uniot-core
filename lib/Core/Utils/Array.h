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

/** @cond */
/**
 * DO NOT DELETE THE "utils" GROUP DEFINITION BELOW.
 * Used to create the Utilities topic in the documentation. If you want to delete this file,
 * please paste the group definition into another utility and delete this one.
 */
/** @endcond */

/**
 * @defgroup utils Utilities
 * @brief Common utility components and functions for the Uniot Core
 *
 * This collection offers reusable utilities optimized for embedded systems
 * in the Uniot Core framework. These components are designed with minimal
 * memory footprint and efficient performance, specifically targeting IoT
 * and resource-constrained embedded applications.
 */

#pragma once

#include <Arduino.h>

#include <new>
#include <type_traits>
#include <utility>

namespace uniot {
/**
 * @brief A simple Array class that manages a dynamically allocated array and its size.
 * @defgroup utils_array Array
 * @ingroup utils
 * @{
 *
 * This class provides a memory-efficient, dynamic array implementation optimized for
 * embedded systems. It handles memory allocation, resizing, and element management while
 * providing a clean interface for array operations.
 *
 * Key features:
 * - Dynamic memory allocation with automatic growth
 * - Move semantics support (but not copy semantics to prevent accidental deep copies)
 * - Bounds checking options for safer access
 * - Optimizations for trivial types
 * - Memory-efficient design with capacity management
 *
 * @tparam T The type of elements stored in the array.
 */
template <typename T>
class Array {
 public:
  /**
   * @brief Constructs an empty Array.
   */
  Array() : mData(nullptr), mSize(0), mCapacity(0) {}

  /**
   * @brief Constructs an Array with the given capacity, allocating memory.
   *
   * @param capacity The number of elements in the array.
   */
  Array(size_t capacity) : mData(nullptr), mSize(0), mCapacity(0) {
    reserve(capacity);
  }

  /**
   * @brief Constructs an Array from an existing C-style array.
   *
   * @param size The number of elements in the array.
   * @param values Pointer to the first element of the array.
   */
  Array(size_t size, const T* values) : mData(nullptr), mSize(0), mCapacity(0) {
    if (values && size > 0) {
      mData = new (std::nothrow) T[size];
      if (!mData) {
        return;
      }

      if constexpr (std::is_trivially_copyable<T>::value) {
        memcpy(reinterpret_cast<void*>(mData), values, sizeof(T) * size);
      } else {
        for (size_t i = 0; i < size; ++i) {
          mData[i] = values[i];
        }
      }

      mSize = size;
      mCapacity = size;
    }
  }

  /**
   * @brief Copy constructor (deleted to prevent accidental copying).
   */
  Array(const Array& other) = delete;

  /**
   * @brief Move constructor.
   *
   * @param other The Array to move from.
   */
  Array(Array&& other) noexcept
      : mData(other.mData), mSize(other.mSize), mCapacity(other.mCapacity) {
    other.mData = nullptr;
    other.mSize = 0;
    other.mCapacity = 0;
  }

  /**
   * @brief Copy assignment operator (deleted to prevent accidental copying).
   */
  Array& operator=(const Array& other) = delete;

  /**
   * @brief Move assignment operator.
   *
   * @param other The Array to move from.
   * @retval Array& Reference to the current Array.
   */
  Array& operator=(Array&& other) noexcept {
    if (this != &other) {
      delete[] mData;

      mData = other.mData;
      mSize = other.mSize;
      mCapacity = other.mCapacity;

      other.mData = nullptr;
      other.mSize = 0;
      other.mCapacity = 0;
    }
    return *this;
  }

  /**
   * @brief Destructor that deallocates the array memory.
   */
  ~Array() {
    delete[] mData;
    mData = nullptr;
    mSize = 0;
    mCapacity = 0;
  }

  /**
   * @brief Provides access to the array elements without bounds checking.
   *
   * @param index The index of the element.
   * @retval T& Reference to the element at the specified index.
   */
  T& operator[](size_t index) {
    return mData[index];
  }

  /**
   * @brief Provides read-only access to the array elements without bounds checking.
   *
   * @param index The index of the element.
   * @retval T& Const reference to the element at the specified index.
   */
  const T& operator[](size_t index) const {
    return mData[index];
  }

  /**
   * @brief Provides access to the array elements with bounds checking.
   *
   * @param index The index of the element.
   * @param outValue Reference to store the retrieved value.
   * @retval true The element was retrieved successfully.
   * @retval false The index is out of bounds.
   */
  bool get(size_t index, T& outValue) const {
    if (index < mSize) {
      outValue = mData[index];
      return true;
    }
    return false;
  }

  /**
   * @brief Sets the value of an element in the array.
   *
   * @param index The index of the element.
   * @param value The value to set.
   * @retval true The element was set successfully.
   * @retval false The index is out of bounds.
   */
  bool set(size_t index, const T& value) {
    if (index < mSize) {
      mData[index] = value;
      return true;
    }
    return false;
  }

  /**
   * @brief Returns the size of the array.
   *
   * @retval size_t The size of the array.
   */
  size_t size() const { return mSize; }

  /**
   * @brief Returns the capacity of the array.
   *
   * @retval size_t The number of elements that can be stored without reallocating.
   */
  size_t capacity() const { return mCapacity; }

  /**
   * @brief Checks if the array is empty.
   *
   * @retval true The array is empty.
   * @retval false The array contains elements.
   */
  bool isEmpty() const { return mSize == 0; }

  /**
   * @brief Retrieves a const pointer to the underlying data.
   *
   * @retval T* Const pointer to the first element of the array.
   */
  const T* raw() const { return mData; }

  /**
   * @brief Reserves memory for at least the specified number of elements.
   *
   * @param newCapacity The desired capacity.
   * @retval true The reservation was successful.
   * @retval false The reservation failed (e.g., memory allocation error).
   */
  bool reserve(size_t newCapacity) {
    if (newCapacity <= mCapacity) {
      return true;
    }

    T* new_data = new (std::nothrow) T[newCapacity];
    if (!new_data) {
      return false;
    }

    // Move existing elements
    for (size_t i = 0; i < mSize; ++i) {
      new_data[i] = std::move(mData[i]);
    }

    // Initialize new elements
    if constexpr (std::is_trivially_default_constructible<T>::value) {
      memset(reinterpret_cast<void*>(new_data + mSize), 0, sizeof(T) * (newCapacity - mSize));
    } else {
      for (size_t i = mSize; i < newCapacity; ++i) {
        new_data[i] = T();
      }
    }

    delete[] mData;
    mData = new_data;
    mCapacity = newCapacity;
    return true;
  }

  /**
   * @brief Adds a new element to the end of the array using copy semantics.
   *
   * @param value The value to add.
   * @retval true The push was successful.
   * @retval false The push failed (e.g., memory allocation error).
   */
  bool push(const T& value) {
    if (mSize >= mCapacity) {
      size_t newCapacity = (mCapacity == 0) ? 1 : mCapacity * 2;
      if (!reserve(newCapacity)) {
        return false;
      }
    }
    mData[mSize++] = value;
    return true;
  }

  /**
   * @brief Adds a new element to the end of the array using move semantics.
   *
   * @param value The value to add.
   * @retval true The push was successful.
   * @retval false The push failed (e.g., memory allocation error).
   */
  bool push(T&& value) {
    if (mSize >= mCapacity) {
      size_t newCapacity = (mCapacity == 0) ? 1 : mCapacity * 2;
      if (!reserve(newCapacity)) {
        return false;
      }
    }
    mData[mSize++] = std::move(value);
    return true;
  }

  /**
   * @brief Clears the array, setting its size to zero.
   */
  void clear() {
    // if constexpr (!std::is_trivially_destructible<T>::value) {
    //   for (size_t i = 0; i < mSize; ++i) {
    //     mData[i].~T();
    //   }
    // }
    mSize = 0;
  }

  /**
   * @brief Reduces the capacity to fit the current size.
   *
   * @retval true The shrink was successful.
   * @retval false The shrink failed (e.g., memory allocation error).
   */
  bool shrink() {
    if (mCapacity == mSize) {
      return true;
    }

    T* new_data = nullptr;
    if (mSize > 0) {
      new_data = new (std::nothrow) T[mSize];
      if (!new_data) {
        return false;
      }

      for (size_t i = 0; i < mSize; ++i) {
        new_data[i] = std::move(mData[i]);
      }
    }

    // if constexpr (!std::is_trivially_destructible<T>::value) {
    //   for (size_t i = mSize; i < mCapacity; ++i) {
    //     mData[i].~T();
    //   }
    // }

    delete[] mData;
    mData = new_data;
    mCapacity = mSize;
    return true;
  }

 private:
  T* mData;
  size_t mSize;
  size_t mCapacity;
};
/** @} */
}  // namespace uniot
