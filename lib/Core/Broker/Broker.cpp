/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2020 Uniot <info.uniot@gmail.com>
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
#include "Broker.h"
#include "Subscriber.h"
#include "Publisher.h"

namespace uniot
{
template <class T_topic, class T_msg>
Broker<T_topic, T_msg>::~Broker()
{
  mSubscribers.forEach([this](Subscriber<T_topic, T_msg> *subscriber) { subscriber->mBrokerQueue.removeOne(this); });
  mPublishers.forEach([this](Publisher<T_topic, T_msg> *publisher) { publisher->mBrokerQueue.removeOne(this); });
}

template <class T_topic, class T_msg>
void Broker<T_topic, T_msg>::connect(IBrokerKitConnection<T_topic, T_msg> *connection)
{
  connection->connect(this);
}

template <class T_topic, class T_msg>
void Broker<T_topic, T_msg>::disconnect(IBrokerKitConnection<T_topic, T_msg> *connection)
{
  connection->disconnect(this);
}

template <class T_topic, class T_msg>
void Broker<T_topic, T_msg>::connect(Publisher<T_topic, T_msg> *publisher)
{
  if (publisher)
  {
    mPublishers.pushUnique(publisher);
    publisher->mBrokerQueue.pushUnique(this);
  }
}

template <class T_topic, class T_msg>
void Broker<T_topic, T_msg>::disconnect(Publisher<T_topic, T_msg> *publisher)
{
  if (publisher)
  {
    mPublishers.removeOne(publisher);
    publisher->mBrokerQueue.removeOne(this);
  }
}

template <class T_topic, class T_msg>
void Broker<T_topic, T_msg>::connect(Subscriber<T_topic, T_msg> *subscriber)
{
  if (subscriber)
  {
    mSubscribers.pushUnique(subscriber);
    subscriber->mBrokerQueue.pushUnique(this);
  }
}

template <class T_topic, class T_msg>
void Broker<T_topic, T_msg>::disconnect(Subscriber<T_topic, T_msg> *subscriber)
{
  if (subscriber)
  {
    mSubscribers.removeOne(subscriber);
    subscriber->mBrokerQueue.removeOne(this);
  }
}

template <class T_topic, class T_msg>
void Broker<T_topic, T_msg>::publish(T_topic topic, T_msg msg)
{
  mEvents.push(std::make_pair(topic, msg));
}

template <class T_topic, class T_msg>
uint8_t Broker<T_topic, T_msg>::execute()
{
  while (!mEvents.isEmpty())
  {
    auto event = mEvents.hardPop();
    mSubscribers.forEach([&](Subscriber<T_topic, T_msg> *subscriber) {
      if (subscriber->isSubscribed(event.first))
      {
        subscriber->onPublish(event.first, event.second);
        yield();
      }
    });
  }
  return 0;
}
} // namespace uniot

template class uniot::Broker<unsigned int, int>;
