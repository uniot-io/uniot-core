/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2020 Uniot <contact@uniot.io>
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

#include <Arduino.h>
#include "EventBus.h"
#include "EventListener.h"
#include "EventEmitter.h"

namespace uniot
{
template <class T_topic, class T_msg>
EventBus<T_topic, T_msg>::~EventBus()
{
  mListeners.forEach([this](EventListener<T_topic, T_msg> *listener) { listener->mEventBusQueue.removeOne(this); });
  mEmitters.forEach([this](EventEmitter<T_topic, T_msg> *emitter) { emitter->mEventBusQueue.removeOne(this); });
}

template <class T_topic, class T_msg>
void EventBus<T_topic, T_msg>::connect(IEventBusKitConnection<T_topic, T_msg> *connection)
{
  connection->connect(this);
}

template <class T_topic, class T_msg>
void EventBus<T_topic, T_msg>::disconnect(IEventBusKitConnection<T_topic, T_msg> *connection)
{
  connection->disconnect(this);
}

template <class T_topic, class T_msg>
void EventBus<T_topic, T_msg>::connect(EventEmitter<T_topic, T_msg> *emitter)
{
  if (emitter)
  {
    mEmitters.pushUnique(emitter);
    emitter->mEventBusQueue.pushUnique(this);
  }
}

template <class T_topic, class T_msg>
void EventBus<T_topic, T_msg>::disconnect(EventEmitter<T_topic, T_msg> *emitter)
{
  if (emitter)
  {
    mEmitters.removeOne(emitter);
    emitter->mEventBusQueue.removeOne(this);
  }
}

template <class T_topic, class T_msg>
void EventBus<T_topic, T_msg>::connect(EventListener<T_topic, T_msg> *listener)
{
  if (listener)
  {
    mListeners.pushUnique(listener);
    listener->mEventBusQueue.pushUnique(this);
  }
}

template <class T_topic, class T_msg>
void EventBus<T_topic, T_msg>::disconnect(EventListener<T_topic, T_msg> *listener)
{
  if (listener)
  {
    mListeners.removeOne(listener);
    listener->mEventBusQueue.removeOne(this);
  }
}

template <class T_topic, class T_msg>
void EventBus<T_topic, T_msg>::emitEvent(T_topic topic, T_msg msg)
{
  mEvents.push(std::make_pair(topic, msg));
}

template <class T_topic, class T_msg>
uint8_t EventBus<T_topic, T_msg>::execute()
{
  while (!mEvents.isEmpty())
  {
    auto event = mEvents.hardPop();
    mListeners.forEach([&](EventListener<T_topic, T_msg> *listener) {
      if (listener->isListeningToEvent(event.first))
      {
        listener->onEventReceived(event.first, event.second);
        yield();
      }
    });
  }
  return 0;
}
} // namespace uniot

template class uniot::EventBus<unsigned int, int>;
