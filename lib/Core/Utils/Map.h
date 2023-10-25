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

#include <Common.h>

#include "IterableQueue.h"

namespace uniot {

template <typename T_Key, typename T_Value>
class Map : public IterableQueue<Pair<T_Key, T_Value>> {
 public:
  using MapItem = Pair<T_Key, T_Value>;

  bool put(const T_Key &key, const T_Value &value) {
    if (exist(key)) {
      return false;
    }
    IterableQueue<MapItem>::push(MakePair(key, value));
    return true;
  }

  T_Value get(const T_Key &key, const T_Value &defaultValue = {}) {
    IterableQueue<MapItem>::begin();
    while (!IterableQueue<MapItem>::isEnd()) {
      auto item = IterableQueue<MapItem>::current();
      if (item.first == key) {
        return item.second;
      }
      IterableQueue<MapItem>::next();
    }
    return defaultValue;
  }

  bool exist(const T_Key &key) {
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

  bool remove(const T_Key &key) {
    IterableQueue<MapItem>::begin();
    while (!IterableQueue<MapItem>::isEnd()) {
      auto item = IterableQueue<MapItem>::current();
      if (item.first == key) {
        return IterableQueue<MapItem>::removeOne(item);
      }
      IterableQueue<MapItem>::next();
    }
    return false;
  }
};

}  // namespace uniot
