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

#include "ClearQueue.h"

/**
 * @brief A queue implementation with iteration capabilities
 * @defgroup utils_iterablequeue IterableQueue
 * @ingroup utils
 *
 * IterableQueue extends ClearQueue to provide functionality for iterating through
 * the elements in the queue without removing them. This allows for queue traversal
 * while maintaining the queue structure intact.
 *
 * @tparam T The type of elements stored in the queue
 * @{
 */
template <typename T>
class IterableQueue : public ClearQueue<T> {
 public:
  /**
   * @brief Initialize the iterator to the beginning of the queue
   *
   * Sets the current position to the head of the queue for iteration.
   */
  void begin() const;

  /**
   * @brief Check if the iterator has reached the end of the queue
   *
   * @retval true If the iterator has reached the end (no more elements)
   * @retval false If there are still elements to iterate through
   */
  bool isEnd() const;

  /**
   * @brief Move to the next element and return the current element
   *
   * Advances the iterator to the next position and returns the value
   * at the previous position.
   *
   * @retval T& The value of the current element before advancing
   */
  const T& next() const;

  /**
   * @brief Access the current element without moving the iterator
   *
   * @retval T& The value at the current iterator position
   */
  const T& current() const;

  /**
   * @brief Remove the current element from the queue
   *
   * Removes the element at the current iterator position and advances
   * the iterator to the next element. If the current element is the last
   * element, the iterator will point to the end.
   *
   * @retval true The element was successfully removed
   * @retval false The iterator was at the end or queue was empty
   */
  bool deleteCurrent();

 protected:
  /**
   * @brief Pointer to the current node during iteration
   *
   * This mutable member allows modification within const methods
   * to support iteration in const contexts.
   */
  mutable typename ClearQueue<T>::pnode mCurrent;
};
/** @} */

template <typename T>
void IterableQueue<T>::begin() const {
  mCurrent = ClearQueue<T>::mHead;
}

template <typename T>
bool IterableQueue<T>::isEnd() const {
  return !mCurrent;
}

template <typename T>
const T& IterableQueue<T>::next() const {
  auto prevCurrent = mCurrent;
  mCurrent = mCurrent->next;
  return prevCurrent->value;
}

template <typename T>
const T& IterableQueue<T>::current() const {
  return mCurrent->value;
}

template <typename T>
bool IterableQueue<T>::deleteCurrent() {
  if (!mCurrent) {
    return false;
  }

  if (mCurrent == ClearQueue<T>::mHead) {
    ClearQueue<T>::hardPop();
    mCurrent = ClearQueue<T>::mHead;
    return true;
  }

  typename ClearQueue<T>::pnode prev = ClearQueue<T>::mHead;
  while (prev && prev->next != mCurrent) {
    prev = prev->next;
  }

  if (!prev) {
    return false;
  }

  auto nextNode = mCurrent->next;
  prev->next = nextNode;

  if (!nextNode) {
    ClearQueue<T>::mTail = prev;
  }

  delete mCurrent;
  mCurrent = nextNode;

  return true;
}
