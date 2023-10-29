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

#include "EventEmitter.h"

#include <Arduino.h>

#include "EventBus.h"

namespace uniot {
template <class T_topic, class T_msg, class T_data>
void EventEmitter<T_topic, T_msg, T_data>::emitEvent(T_topic topic, T_msg msg) {
  this->mEventBusQueue.forEach([&](EventBus<T_topic, T_msg, T_data> *eventBus) {
    eventBus->emitEvent(topic, msg);
    yield();
  });
}
}  // namespace uniot

template class uniot::EventEmitter<unsigned int, int, Bytes>;
