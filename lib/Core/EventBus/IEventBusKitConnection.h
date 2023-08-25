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

#include "EventBus.h"

namespace uniot
{
template <class T_topic, class T_msg>
class EventBus;

template <class T_topic, class T_msg>
class IEventBusKitConnection
{
public:
  virtual ~IEventBusKitConnection() {}
  virtual void connect(EventBus<T_topic, T_msg> *eventBus) = 0;
  virtual void disconnect(EventBus<T_topic, T_msg> *eventBus) = 0;
};

using ICoreEventBusKitConnection = IEventBusKitConnection<unsigned int, int>;
} // namespace uniot
