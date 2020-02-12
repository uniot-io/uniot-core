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

#pragma once

#include <IExecutor.h>
#include <ClearQueue.h>
#include "IBrokerKitConnection.h"

namespace uniot
{
template <class T_topic, class T_msg>
class Subscriber;

template <class T_topic, class T_msg>
class Publisher;

template <class T_topic, class T_msg>
class Broker : public uniot::IExecutor
{
  friend class Subscriber<T_topic, T_msg>;
  friend class Publisher<T_topic, T_msg>;

public:
  virtual ~Broker();

  void connect(IBrokerKitConnection<T_topic, T_msg> *connection);
  void disconnect(IBrokerKitConnection<T_topic, T_msg> *connection);

  void connect(Publisher<T_topic, T_msg> *publisher);
  void disconnect(Publisher<T_topic, T_msg> *publisher);

  void connect(Subscriber<T_topic, T_msg> *subscriber);
  void disconnect(Subscriber<T_topic, T_msg> *subscriber);

  void publish(T_topic topic, T_msg msg);
  virtual uint8_t execute() override;

private:
  ClearQueue<Subscriber<T_topic, T_msg> *> mSubscribers; // TODO: need real set; std::set is broken into esp xtensa sdk
  ClearQueue<Publisher<T_topic, T_msg> *> mPublishers;
  ClearQueue<std::pair<T_topic, T_msg>> mEvents;
};

using GeneralBroker = Broker<unsigned int, int>;
} // namespace uniot
