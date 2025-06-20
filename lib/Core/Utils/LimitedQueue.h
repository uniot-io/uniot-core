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

/**
 * @brief A queue with a maximum size limit that automatically removes oldest elements when full
 * @defgroup utils_limitedqueue LimitedQueue
 * @ingroup utils
 *
 * LimitedQueue extends ClearQueue with capacity limiting functionality. When the queue
 * reaches its size limit and a new element is added, the oldest element is automatically
 * removed to maintain the size constraint.
 *
 * @tparam T The type of elements stored in the queue
 * @{
 */
template <typename T>
class LimitedQueue : public ClearQueue<T> {
 public:
  /**
   * @brief Construct a new LimitedQueue with default settings
   *
   * Creates an empty queue with a size limit of 0 (unlimited by default)
   */
  LimitedQueue()
      : ClearQueue<T>(), mLimit(0), mSize(0) {}

  /**
   * @brief Get the current size limit of the queue
   *
   * @retval size_t The maximum number of elements the queue can hold
   */
  inline size_t limit() const {
    return mLimit;
  }

  /**
   * @brief Get the current number of elements in the queue
   *
   * @retval size_t The current number of elements
   */
  inline size_t size() const {
    return mSize;
  }

  /**
   * @brief Set the maximum size limit for the queue
   *
   * If the new limit is smaller than the current size,
   * oldest elements are automatically removed.
   *
   * @param limit The maximum number of elements the queue can hold
   */
  void limit(size_t limit) {
    limit = limit < 0 ? 0 : limit;
    mLimit = limit;
    applyLimit();
  }

  /**
   * @brief Check if the queue has reached its size limit
   *
   * @retval true If the queue is full (size >= limit)
   * @retval false If the queue has space for more elements
   */
  inline bool isFull() const {
    return mSize >= mLimit;
  }

  /**
   * @brief Enforce the size limit by removing oldest elements if necessary
   *
   * Removes elements from the front of the queue until the size is within limits
   */
  void applyLimit() {
    for (; mSize > mLimit; --mSize) {
      ClearQueue<T>::hardPop();
    }
  }

  /**
   * @brief Add an element to the queue while respecting the size limit
   *
   * If adding the element would exceed the size limit, the oldest element is removed first
   *
   * @param value The element to add to the queue
   */
  void pushLimited(const T &value) {
    ClearQueue<T>::push(value);
    mSize++;
    applyLimit();
  }

  /**
   * @brief Remove and return the oldest element from the queue
   *
   * @param errorCode Value to return if the queue is empty
   * @retval T The removed element
   * @retval errorCode The value to return if the queue is empty
   */
  T popLimited(const T &errorCode) {
    if (mSize) {
      mSize--;
    }
    return ClearQueue<T>::pop(errorCode);
  }

  /**
   * @brief Recalculate the size by traversing the queue
   *
   * This method should be used if the size tracking may have become inconsistent
   *
   * @retval size_t The actual number of elements in the queue
   */
  size_t calcSize() const {
    mSize = ClearQueue<T>::calcSize();
    return mSize;
  }

  /**
   * @brief Remove all elements from the queue
   *
   * Resets the queue to its initial empty state
   */
  void clean() {
    ClearQueue<T>::clean();
    mSize = 0;
  }

 private:
  size_t mLimit;  ///< Maximum number of elements the queue can hold
  size_t mSize;   ///< Current number of elements in the queue
};
/** @} */
