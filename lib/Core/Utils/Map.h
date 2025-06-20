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

#include <Common.h>

#include "IterableQueue.h"

namespace uniot {

/**
 * @brief A simple map class that associates keys with values using an iterable queue.
 * @defgroup utils_map Map
 * @ingroup utils
 *
 * The Map class provides a key-value storage mechanism built on top of the IterableQueue.
 * It allows storing, retrieving, checking existence, and removing key-value pairs.
 * This implementation does not allow duplicate keys.
 *
 * @tparam T_Key The type of the keys.
 * @tparam T_Value The type of the values.
 * @{
 */
template <typename T_Key, typename T_Value>
class Map : public IterableQueue<Pair<T_Key, T_Value>> {
 public:
  /**
   * @brief Type alias for a key-value pair in the map.
   */
  using MapItem = Pair<T_Key, T_Value>;

  /**
   * @brief Inserts a key-value pair into the map.
   *
   * This method checks if the key already exists in the map.
   * If not, it adds a new key-value pair to the map.
   *
   * @param key The key to insert.
   * @param value The value to associate with the key.
   * @retval true Insertion was successful.
   * @retval false The key already exists in the map.
   */
  bool put(const T_Key& key, const T_Value& value) {
    if (exist(key)) {
      return false;
    }
    IterableQueue<MapItem>::push(MakePair(key, value));
    return true;
  }

  /**
   * @brief Retrieves the value associated with a key.
   *
   * This method iterates through the map to find the key.
   * If found, it returns the associated value; otherwise, it returns the default value.
   *
   * @param key The key to search for.
   * @param defaultValue The default value to return if the key is not found.
   * @retval value The value associated with the key.
   * @retval defaultValue The default value if the key is not found.
   */
  const T_Value& get(const T_Key& key, const T_Value& defaultValue = {}) const {
    IterableQueue<MapItem>::begin();
    while (!IterableQueue<MapItem>::isEnd()) {
      auto &item = IterableQueue<MapItem>::current();
      if (item.first == key) {
        return item.second;
      }
      IterableQueue<MapItem>::next();
    }
    return defaultValue;
  }

  /**
   * @brief Checks if a key exists in the map.
   *
   * This method iterates through all key-value pairs in the map
   * to determine if the specified key exists.
   *
   * @param key The key to check for.
   * @retval true The key exists in the map.
   * @retval false The key does not exist in the map.
   */
  bool exist(const T_Key& key) const {
    IterableQueue<MapItem>::begin();
    while (!IterableQueue<MapItem>::isEnd()) {
      auto item = IterableQueue<MapItem>::current();
      if (item.first == key) {
        return true;
      }
      IterableQueue<MapItem>::next();
    }
    return false;
  }

  /**
   * @brief Removes a key-value pair from the map.
   *
   * This method searches for the specified key and removes the
   * corresponding key-value pair if found.
   *
   * @param key The key of the pair to remove.
   * @retval true The key-value pair was successfully removed.
   * @retval false The key was not found in the map.
   */
  bool remove(const T_Key& key) {
    IterableQueue<MapItem>::begin();
    while (!IterableQueue<MapItem>::isEnd()) {
      const auto& item = IterableQueue<MapItem>::current();
      if (item.first == key) {
        return IterableQueue<MapItem>::deleteCurrent();
      }
      IterableQueue<MapItem>::next();
    }
    return false;
  }
};
/** @} */

}  // namespace uniot
