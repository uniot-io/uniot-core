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

#include <Arduino.h>

#include <functional>

/**
 * std::queue requires much more resources
 */
template <typename T>
class ClearQueue {
 public:
  typedef std::function<void(const T &)> VoidCallback;

  ClearQueue(ClearQueue const &) = delete;
  void operator=(ClearQueue const &) = delete;

  ClearQueue();
  virtual ~ClearQueue();

  void push(const T &value);
  bool pushUnique(const T &value);
  T hardPop();
  const T &hardPeek() const;
  T pop(const T &errorCode);
  const T &peek(const T &errorCode) const;
  bool removeOne(const T &value);
  bool contains(const T &value) const;
  T *find(const T &value) const;
  inline bool isEmpty() const;
  void clean();
  void forEach(VoidCallback callback) const;

 protected:
  typedef struct node {
    T value;
    node *next;
  } *pnode;

  pnode mHead;
  pnode mTail;
};

template <typename T>
ClearQueue<T>::ClearQueue() {
  mHead = nullptr;
  mTail = nullptr;
}

template <typename T>
ClearQueue<T>::~ClearQueue() {
  clean();
}

template <typename T>
bool ClearQueue<T>::pushUnique(const T &value) {
  if (!contains(value)) {
    push(value);
    return true;
  }
  return false;
}

template <typename T>
void ClearQueue<T>::push(const T &value) {
  pnode cur = mTail;
  mTail = (pnode) new node;

  mTail->next = nullptr;
  mTail->value = value;
  if (isEmpty()) {
    mHead = mTail;
  } else {
    cur->next = mTail;
  }
}

template <typename T>
T ClearQueue<T>::hardPop() {
  T value = mHead->value;
  pnode cur = mHead->next;
  delete mHead;
  mHead = cur;

  if (mHead == nullptr) {
    mTail = nullptr;
  }

  return value;
}

template <typename T>
const T &ClearQueue<T>::hardPeek() const {
  return mHead->value;
}

template <typename T>
T ClearQueue<T>::pop(const T &errorCode) {
  if (!isEmpty()) {
    return hardPop();
  }
  return errorCode;
}

template <typename T>
const T &ClearQueue<T>::peek(const T &errorCode) const {
  if (!isEmpty()) {
    return hardPeek();
  }
  return errorCode;
}

template <typename T>
bool ClearQueue<T>::removeOne(const T &value) {
  if (!isEmpty()) {
    if (mHead->value == value) {
      hardPop();
      return true;
    }
    for (pnode cur = mHead; cur->next != nullptr; cur = cur->next) {
      if (cur->next->value == value) {
        pnode newNext = cur->next->next;
        delete cur->next;
        cur->next = newNext;

        if (newNext == nullptr) {
          mTail = cur;
        }

        return true;
      }
    }
  }
  return false;
}

template <typename T>
bool ClearQueue<T>::contains(const T &value) const {
  for (pnode cur = mHead; cur != nullptr; cur = cur->next) {
    if (cur->value == value) {
      return true;
    }
  }
  return false;
}

template <typename T>
T *ClearQueue<T>::find(const T &value) const {
  for (pnode cur = mHead; cur != nullptr; cur = cur->next) {
    if (cur->value == value) {
      return &(cur->value);
    }
  }
  return nullptr;
}

template <typename T>
inline bool ClearQueue<T>::isEmpty() const {
  return mHead == nullptr;
}

template <typename T>
void ClearQueue<T>::clean() {
  for (pnode cur = mHead; cur != nullptr; mHead = cur) {
    cur = mHead->next;
    delete mHead;
  }

  mHead = nullptr;
  mTail = nullptr;
}

template <typename T>
void ClearQueue<T>::forEach(VoidCallback callback) const {
  for (pnode cur = mHead; cur != nullptr; cur = cur->next) {
    callback(cur->value);
  }
}
