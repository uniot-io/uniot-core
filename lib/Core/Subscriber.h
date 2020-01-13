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

#pragma once

#include <ClearQueue.h>

template<class T_topic, class T_msg>
class Broker;

template<class T_topic, class T_msg>
class Subscriber
{
  friend class Broker<T_topic, T_msg>;

public:
  virtual ~Subscriber();

  Subscriber* subscribe(T_topic topic);
  Subscriber* unsubscribe(T_topic topic);
  bool isSubscribed(T_topic topic);
  void connect(Broker<T_topic, T_msg>* broker);
  void disconnect(Broker<T_topic, T_msg>* broker);
  virtual void onPublish(T_topic topic, T_msg msg) = 0;
  
private:
  ClearQueue<Broker<T_topic, T_msg>*> mBrokerQueue;
  ClearQueue<T_topic> mTopics;
};

