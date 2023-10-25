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
#include <IterableQueue.h>

#include "EventEntity.h"

namespace uniot {
template <class T_topic, class T_msg, class T_data>
class EventBus;

template <class T_topic, class T_msg, class T_data>
class EventEmitter : public EventEntity<T_topic, T_msg, T_data> {
  friend class EventBus<T_topic, T_msg, T_data>;

 public:
  virtual ~EventEmitter();

  void emitEvent(T_topic topic, T_msg msg);
  bool sendDataToChannel(T_topic channel, T_data data);
};

using CoreEventEmitter = EventEmitter<unsigned int, int, Bytes>;
}  // namespace uniot
