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
 * @brief A lightweight queue implementation optimized for resource-constrained environments
 * @defgroup utils_clearqueue ClearQueue
 * @ingroup utils
 *
 * ClearQueue is designed as a simpler alternative to std::queue that requires fewer resources,
 * making it suitable for embedded systems and IoT devices. It implements a singly linked list
 * to provide queue functionality with additional operations like unique insertion and element removal.
 *
 * @tparam T Type of elements stored in the queue
 * @{
 */
template <typename T>
class ClearQueue {
 public:
  /**
   * @brief Callback function type for forEach operations
   */
  typedef std::function<void(const T &)> VoidCallback;

  /**
   * @brief Deleted copy constructor to prevent copying
   */
  ClearQueue(ClearQueue const &) = delete;

  /**
   * @brief Deleted assignment operator to prevent copying
   */
  void operator=(ClearQueue const &) = delete;

  /**
   * @brief Constructs an empty queue
   */
  ClearQueue();

  /**
   * @brief Destroys the queue and releases all allocated memory
   */
  virtual ~ClearQueue();

  /**
   * @brief Adds an element to the end of the queue
   *
   * @param value The value to add to the queue
   */
  void push(const T &value);

  /**
   * @brief Adds an element to the queue only if it doesn't already exist
   *
   * @param value The value to add to the queue
   * @retval true The element was added to the queue
   * @retval false The element already exists in the queue
   */
  bool pushUnique(const T &value);

  /**
   * @brief Removes and returns the element at the front of the queue
   *
   * @warning This method assumes the queue is not empty. Call isEmpty() before using.
   * @retval T The front element of the queue
   */
  T hardPop();

  /**
   * @brief Returns the element at the front of the queue without removing it
   *
   * @warning This method assumes the queue is not empty. Call isEmpty() before using.
   * @retval T& Reference to the front element of the queue
   */
  const T &hardPeek() const;

  /**
   * @brief Safely removes and returns the element at the front of the queue
   *
   * @param errorCode Value to return if the queue is empty
   * @retval T The front element of the queue
   * @retval errorCode The value to return if the queue is empty
   */
  T pop(const T &errorCode);

  /**
   * @brief Safely returns the element at the front of the queue without removing it
   *
   * @param errorCode Value to return if the queue is empty
   * @retval T& Reference to the front element of the queue
   * @retval errorCode The value to return if the queue is empty
   */
  const T &peek(const T &errorCode) const;

  /**
   * @brief Removes the first occurrence of a specific value from the queue
   *
   * @param value The value to remove
   * @retval true The value was found and removed
   * @retval false The value was not found in the queue
   */
  bool removeOne(const T &value);

  /**
   * @brief Checks if the queue contains a specific value
   *
   * @param value The value to search for
   * @retval true The value exists in the queue
   * @retval false The value does not exist in the queue
   */
  bool contains(const T &value) const;

  /**
   * @brief Finds and returns a pointer to the first occurrence of a value
   *
   * @param value The value to search for
   * @retval T* Pointer to the found element
   * @retval nullptr The value was not found in the queue
   */
  T *find(const T &value) const;

  /**
   * @brief Checks if the queue is empty
   *
   * @retval true The queue is empty
   * @retval false The queue contains elements
   */
  inline bool isEmpty() const;

  /**
   * @brief Removes all elements from the queue
   */
  void clean();

  /**
   * @brief Executes a callback function on each element in the queue
   *
   * @param callback Function to execute on each element
   */
  void forEach(VoidCallback callback) const;

 protected:
  /**
   * @brief Node structure for the linked list implementation
   */
  typedef struct node {
    T value;     ///< The stored value
    node *next;  ///< Pointer to the next node
  } *pnode;

  pnode mHead;  ///< Pointer to the first node in the queue
  pnode mTail;  ///< Pointer to the last node in the queue
};
/** @} */

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
