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

#include <ClearQueue.h>

template<typename T>
class ArrayBuilder
{
public:
  ArrayBuilder()
  : mpQueue(nullptr), mpArray(nullptr), mSize(0) { }

  ArrayBuilder<T>* reset() {
    _clean();
    mpQueue = new ClearQueue<T*>();
    return this;
  }

  ArrayBuilder<T>* push(T* pObject) {
    if(mpQueue) {
      mpQueue->push(pObject);
      mSize++;
    }
    return this;
  }

  T* build() {
    if(mpQueue && mSize) {
      _cleanArray();
      mpArray = new T[mSize];
      uint8_t idx = 0;
      mpQueue->forEach([this, &idx](T* pObject) { mpArray[idx++] = *pObject; } );
      _cleanQueue();
    }
    return mpArray;
  }

  size_t size() {
    return mSize;
  }

  ~ArrayBuilder() {
    _clean();
  }

private:
  void _cleanQueue() {
    if(mpQueue) {
      delete mpQueue;
      mpQueue = nullptr;
    }
  }

  void _cleanArray() {
    if(mpArray) {
      delete mpArray;
      mpArray = nullptr;
    }
  }

  void _clean() {
    mSize = 0;
    _cleanQueue();
    _cleanArray();
  }

  ClearQueue<T*>* mpQueue;
  T* mpArray;
  size_t mSize;
};