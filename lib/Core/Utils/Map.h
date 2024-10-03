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
 *
 * @tparam T_Key The type of the keys.
 * @tparam T_Value The type of the values.
 */
template <typename T_Key, typename T_Value>
class Map : public IterableQueue<Pair<T_Key, T_Value>> {
 public:
  using MapItem = Pair<T_Key, T_Value>;

  /**
   * @brief Inserts a key-value pair into the map.
   *
   * @param key The key to insert.
   * @param value The value to associate with the key.
   * @return true if insertion was successful, false if the key already exists.
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
   * @param key The key to search for.
   * @param defaultValue The default value to return if the key is not found.
   * @return The associated value if found, otherwise defaultValue.
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
   * @param key The key to check for.
   * @return true if the key exists, false otherwise.
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
   * @param key The key of the pair to remove.
   * @return true if removal was successful, false if the key was not found.
   */
  bool remove(const T_Key& key) {
    IterableQueue<MapItem>::begin();
    while (!IterableQueue<MapItem>::isEnd()) {
      auto& item = IterableQueue<MapItem>::current();
      if (item.first == key) {
        return IterableQueue<MapItem>::removeOne(item);
      }
      IterableQueue<MapItem>::next();
    }
    return false;
  }
};

}  // namespace uniot
