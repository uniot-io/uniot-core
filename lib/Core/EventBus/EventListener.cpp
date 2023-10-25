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

#include "EventListener.h"

#include <Arduino.h>
#include <Logger.h>

#include "EventBus.h"

namespace uniot {
template <class T_topic, class T_msg, class T_data>
EventListener<T_topic, T_msg, T_data>::~EventListener() {
  this->mEventBusQueue.forEach([this](EventBus<T_topic, T_msg, T_data> *eventBus) {
    eventBus->mListeners.removeOne(this);
  });
}

template <class T_topic, class T_msg, class T_data>
EventListener<T_topic, T_msg, T_data> *EventListener<T_topic, T_msg, T_data>::listenToEvent(T_topic topic) {
  mTopics.pushUnique(topic);
  return this;
}

template <class T_topic, class T_msg, class T_data>
EventListener<T_topic, T_msg, T_data> *EventListener<T_topic, T_msg, T_data>::stopListeningToEvent(T_topic topic) {
  mTopics.removeOne(topic);
  return this;
}

template <class T_topic, class T_msg, class T_data>
bool EventListener<T_topic, T_msg, T_data>::isListeningToEvent(T_topic topic) {
  return mTopics.contains(topic);
}

template <class T_topic, class T_msg, class T_data>
void EventListener<T_topic, T_msg, T_data>::receiveDataFromChannel(T_topic channel, DataChannelCallback callback) {
  this->mEventBusQueue.forEach([&](EventBus<T_topic, T_msg, T_data> *eventBus) {
    if (callback) {
      auto empty = eventBus->mDataChannels.isEmpty(channel);
      auto data = eventBus->mDataChannels.receive(channel);
      callback(eventBus->mId, empty, data);
    }
    yield();
  });
}
}  // namespace uniot

template class uniot::EventListener<unsigned int, int, Bytes>;
