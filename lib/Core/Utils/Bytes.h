/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2020 Uniot <contact@uniot.io>
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
#include <type_traits>
class Bytes
{
public:

  Bytes() : Bytes(nullptr, 0)
  {
    
  }

  Bytes(const uint8_t* data, size_t size) {
    _init();
    if (data) {
      _copy(data, size);
    } else if (size) {
      _reserve(size);
    }
  }

  Bytes(const char *str) : Bytes((uint8_t *)str, strlen(str) + 1) // + 1 null terminator
  {}

  template <typename T>
  Bytes(T value) : Bytes(((uint8_t *) &value), sizeof(T))
  {
    static_assert(std::is_fundamental<T>::value, "only fundamental types are allowed");
  }

  Bytes(const Bytes &value) {
    _init();
    *this = value;
  }

  virtual ~Bytes() {
    _invalidate();
  }

  Bytes & operator =(const Bytes &rhs) {
    if(this != &rhs) {
      if(rhs.mBuffer) {
        _copy(rhs.mBuffer, rhs.mSize);
      } else {
        _invalidate();
      }
    }
    return *this;
  }

  Bytes & operator =(const String &rhs) {
    if(rhs.length()) {
      _copy((const uint8_t *)rhs.c_str(), rhs.length());
    } else {
      _invalidate();
    }
    return *this;
  }

  template <typename T>
  operator T() {
    return *((T *)mBuffer);
  }

  uint8_t* raw() const {
    return mBuffer;
  }

  Bytes &terminate()
  {
    if (mBuffer[mSize - 1] != '\0') {
      _reserve(mSize + 1);
      mBuffer[mSize - 1] = '\0';
    }
    return *this;
  }

  const char *c_str() const {
    return (const char *)mBuffer;
  }

  size_t size() const {
    return mSize;
  }

  void clean() {
    _invalidate();
  }

private:

  inline void _init(void) {
    mBuffer = nullptr;
    mSize = 0;
  }

  void _invalidate(void) {
    if(mBuffer) {
      free(mBuffer);
    }
    _init();
  }

  bool _reserve(size_t newSize) {
    mBuffer = (uint8_t*) realloc(mBuffer, newSize);
    if(mBuffer && (newSize > mSize)) {
      memset(mBuffer + mSize, 0, newSize - mSize);
    }
    mSize = newSize;
    return mBuffer;
  }

  Bytes & _copy(const uint8_t* data, size_t size) {
    if(_reserve(size)) {
      memcpy(mBuffer, data, size);
    } else {
      _invalidate();
    }
    return *this;
  }

  uint8_t* mBuffer;
  size_t mSize;
};