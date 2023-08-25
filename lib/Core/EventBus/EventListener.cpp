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
#include "EventListener.h"
#include "EventBus.h"

namespace uniot
{
template <class T_topic, class T_msg>
EventListener<T_topic, T_msg>::~EventListener()
{
  mEventBusQueue.forEach([this](EventBus<T_topic, T_msg> *eventBus) { eventBus->mListeners.removeOne(this); });
}

template <class T_topic, class T_msg>
EventListener<T_topic, T_msg> *EventListener<T_topic, T_msg>::listenToEvent(T_topic topic)
{
  mTopics.pushUnique(topic);
  return this;
}

template <class T_topic, class T_msg>
EventListener<T_topic, T_msg> *EventListener<T_topic, T_msg>::stopListeningToEvent(T_topic topic)
{
  mTopics.removeOne(topic);
  return this;
}

template <class T_topic, class T_msg>
bool EventListener<T_topic, T_msg>::isListeningToEvent(T_topic topic)
{
  return mTopics.contains(topic);
}

template <class T_topic, class T_msg>
void EventListener<T_topic, T_msg>::connect(EventBus<T_topic, T_msg> *eventBus)
{
  if (eventBus)
  {
    eventBus->connect(this);
  }
}

template <class T_topic, class T_msg>
void EventListener<T_topic, T_msg>::disconnect(EventBus<T_topic, T_msg> *eventBus)
{
  if (eventBus)
  {
    eventBus->disconnect(this);
  }
}
} // namespace uniot

template class uniot::EventListener<unsigned int, int>;
