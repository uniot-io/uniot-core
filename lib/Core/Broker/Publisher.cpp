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
#include "Publisher.h"
#include "Broker.h"

namespace uniot
{
template <class T_topic, class T_msg>
Publisher<T_topic, T_msg>::~Publisher()
{
  mBrokerQueue.forEach([this](Broker<T_topic, T_msg> *broker) { broker->mPublishers.removeOne(this); });
}

template <class T_topic, class T_msg>
void Publisher<T_topic, T_msg>::publish(T_topic topic, T_msg msg)
{
  mBrokerQueue.forEach([&](Broker<T_topic, T_msg> *broker) { broker->publish(topic, msg); yield(); });
}

template <class T_topic, class T_msg>
void Publisher<T_topic, T_msg>::connect(Broker<T_topic, T_msg> *broker)
{
  if (broker)
  {
    broker->connect(this);
  }
}

template <class T_topic, class T_msg>
void Publisher<T_topic, T_msg>::disconnect(Broker<T_topic, T_msg> *broker)
{
  if (broker)
  {
    broker->disconnect(this);
  }
}
} // namespace uniot

template class uniot::Publisher<unsigned int, int>;
