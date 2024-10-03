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

#include "ClearQueue.h"

template <typename T>
class LimitedQueue : public ClearQueue<T> {
 public:
  LimitedQueue()
      : ClearQueue<T>(), mLimit(0), mSize(0) {}

  inline size_t limit() const {
    return mLimit;
  }

  inline size_t size() const {
    return mSize;
  }

  void limit(size_t limit) {
    limit = limit < 0 ? 0 : limit;
    mLimit = limit;
    applyLimit();
  }

  inline bool isFull() const {
    return mSize >= mLimit;
  }

  void applyLimit() {
    for (; mSize > mLimit; --mSize) {
      ClearQueue<T>::hardPop();
    }
  }

  void pushLimited(const T &value) {
    ClearQueue<T>::push(value);
    mSize++;
    applyLimit();
  }

  T popLimited(const T &errorCode) {
    if (mSize) {
      mSize--;
    }
    return ClearQueue<T>::pop(errorCode);
  }

  size_t calcSize() {
    mSize = 0;
    typename ClearQueue<T>::pnode cur = ClearQueue<T>::mHead;
    while (cur != nullptr) {
      cur = cur->next;
      mSize++;
    }
    return mSize;
  }

 private:
  size_t mLimit;
  size_t mSize;
};
