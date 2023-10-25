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
#include "DataChannels.h"

#include <Logger.h>

namespace uniot {

template <class T_channel, class T_data>
bool DataChannels<T_channel, T_data>::open(T_channel channel, size_t limit) {
  if (!mChannels.exist(channel)) {
    mChannels.put(channel, std::make_shared<LimitedQueue<T_data>>());
    mChannels.get(channel)->limit(limit);
    return true;
  }
  return false;
}

template <class T_channel, class T_data>
bool DataChannels<T_channel, T_data>::close(T_channel channel) {
  return mChannels.remove(channel);
}

template <class T_channel, class T_data>
bool DataChannels<T_channel, T_data>::send(T_channel channel, T_data data) {
  if (mChannels.exist(channel)) {
    UNIOT_LOG_WARN_IF(mChannels.get(channel)->isFull(), "Channel `%d` is full. Overwriting oldest data.", channel);
    mChannels.get(channel)->pushLimited(data);
    return true;
  }
  return false;
}

template <class T_channel, class T_data>
T_data DataChannels<T_channel, T_data>::receive(T_channel channel) {
  if (mChannels.exist(channel)) {
    return mChannels.get(channel)->popLimited({});
  }
  return {};
}

template <class T_channel, class T_data>
bool DataChannels<T_channel, T_data>::isEmpty(T_channel channel) {
  if (mChannels.exist(channel)) {
    return mChannels.get(channel)->isEmpty();
  }
  return true;
}

}  // namespace uniot

template class uniot::DataChannels<unsigned int, Bytes>;
