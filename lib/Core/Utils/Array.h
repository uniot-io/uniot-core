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

#include <new>
#include <type_traits>
#include <utility>

namespace uniot {

/**
 * @brief A simple Array class that manages a dynamically allocated array and its size.
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
   * @return Reference to the current Array.
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
   * @return Reference to the element at the specified index.
   */
  T& operator[](size_t index) {
    return mData[index];
  }

  /**
   * @brief Provides read-only access to the array elements without bounds checking.
   *
   * @param index The index of the element.
   * @return Const reference to the element at the specified index.
   */
  const T& operator[](size_t index) const {
    return mData[index];
  }

  /**
   * @brief Provides access to the array elements with bounds checking.
   *
   * @param index The index of the element.
   * @param outValue Reference to store the retrieved value.
   * @return true if the element was retrieved successfully, false otherwise.
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
   * @return true if the element was set successfully, false otherwise.
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
   * @return The number of elements in the array.
   */
  size_t size() const { return mSize; }

  /**
   * @brief Returns the capacity of the array.
   *
   * @return The number of elements that can be stored without reallocating.
   */
  size_t capacity() const { return mCapacity; }

  /**
   * @brief Checks if the array is empty.
   *
   * @return true if the array is empty, false otherwise.
   */
  bool isEmpty() const { return mSize == 0; }

  /**
   * @brief Retrieves a const pointer to the underlying data.
   *
   * @return Const pointer to the first element of the array.
   */
  const T* raw() const { return mData; }

  /**
   * @brief Reserves memory for at least the specified number of elements.
   *
   * @param newCapacity The desired capacity.
   * @return true if the reservation was successful, false otherwise.
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
   * @return true if the push was successful, false otherwise.
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
   * @return true if the push was successful, false otherwise.
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
   * @return true if the operation was successful, false otherwise.
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

}  // namespace uniot
