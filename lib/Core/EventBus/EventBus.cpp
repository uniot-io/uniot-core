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

#include "EventBus.h"

#include <Arduino.h>

#include "EventEmitter.h"
#include "EventEntity.h"
#include "EventEntityType.h"
#include "EventListener.h"

namespace uniot {
template <class T_topic, class T_msg, class T_data>
EventBus<T_topic, T_msg, T_data>::~EventBus() {
  mEntities.forEach([this](EventEntity<T_topic, T_msg, T_data> *entity) { entity->mEventBusQueue.removeOne(this); });
}

template <class T_topic, class T_msg, class T_data>
void EventBus<T_topic, T_msg, T_data>::registerKit(IEventBusConnectionKit<T_topic, T_msg, T_data> &connection) {
  connection.registerWithBus(*this);
}

template <class T_topic, class T_msg, class T_data>
void EventBus<T_topic, T_msg, T_data>::unregisterKit(IEventBusConnectionKit<T_topic, T_msg, T_data> &connection) {
  connection.unregisterFromBus(*this);
}

template <class T_topic, class T_msg, class T_data>
bool EventBus<T_topic, T_msg, T_data>::registerEntity(EventEntity<T_topic, T_msg, T_data> *entity) {
  if (entity && entity->connectUnique(this)) {
    mEntities.pushUnique(entity);
    return true;
  }
  return false;
}

template <class T_topic, class T_msg, class T_data>
void EventBus<T_topic, T_msg, T_data>::unregisterEntity(EventEntity<T_topic, T_msg, T_data> *entity) {
  if (entity) {
    mEntities.removeOne(entity);
    entity->mEventBusQueue.removeOne(this);
  }
}

template <class T_topic, class T_msg, class T_data>
bool EventBus<T_topic, T_msg, T_data>::openDataChannel(T_topic topic, size_t limit) {
  return mDataChannels.open(topic, limit);
}

template <class T_topic, class T_msg, class T_data>
bool EventBus<T_topic, T_msg, T_data>::closeDataChannel(T_topic topic) {
  return mDataChannels.close(topic);
}

template <class T_topic, class T_msg, class T_data>
bool EventBus<T_topic, T_msg, T_data>::sendDataToChannel(T_topic topic, T_data data) {
  return mDataChannels.send(topic, data);
}

template <class T_topic, class T_msg, class T_data>
T_data EventBus<T_topic, T_msg, T_data>::receiveDataFromChannel(T_topic topic) {
  return mDataChannels.receive(topic);
}

template <class T_topic, class T_msg, class T_data>
bool EventBus<T_topic, T_msg, T_data>::isDataChannelEmpty(T_topic topic) {
  return mDataChannels.isEmpty(topic);
}

template <class T_topic, class T_msg, class T_data>
void EventBus<T_topic, T_msg, T_data>::emitEvent(T_topic topic, T_msg msg) {
  mEvents.push(MakePair(topic, msg));
}

template <class T_topic, class T_msg, class T_data>
uint8_t EventBus<T_topic, T_msg, T_data>::execute() {
  while (!mEvents.isEmpty()) {
    auto event = mEvents.hardPop();
    // NOTE: Is it worth making a separate list for listeners to reduce the number of iterations?
    // Which is better - saving RAM or CPU time?
    mEntities.forEach([&](EventEntity<T_topic, T_msg, T_data> *entity) {
      if (entity->getType() == EventEntityType::EventListener) {
        // NOTE: This is a hack to avoid `dynamic_cast`.
        // The `dynamic_cast` operation requires RTTI to determine the dynamic type of an object at runtime,
        // but in many embedded systems, RTTI is disabled by default to save memory and reduce code size.
        auto *listener = static_cast<EventListener<T_topic, T_msg, T_data> *>(entity);
        if (listener->isListeningToEvent(event.first)) {
          listener->onEventReceived(event.first, event.second);
        }
      }
      yield();
    });
  }
  return 0;
}
}  // namespace uniot

template class uniot::EventBus<unsigned int, int, Bytes>;
