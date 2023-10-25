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

#include <Bytes.h>
#include <LimitedQueue.h>
#include <Map.h>

namespace uniot {
template <class T_channel, class T_data>
class DataChannels {
 public:
  virtual ~DataChannels() = default;

  bool open(T_channel channel, size_t limit);
  bool close(T_channel channel);
  bool send(T_channel channel, T_data data);
  T_data receive(T_channel channel);
  bool isEmpty(T_channel channel);

 private:
  Map<T_channel, SharedPointer<LimitedQueue<T_data>>> mChannels;
};
}  // namespace uniot
