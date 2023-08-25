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
#include "IEventBusKitConnection.h"

namespace uniot
{
template <class T_topic, class T_msg>
class EventListener;

template <class T_topic, class T_msg>
class EventEmitter;

template <class T_topic, class T_msg>
class EventBus : public uniot::IExecutor
{
  friend class EventListener<T_topic, T_msg>;
  friend class EventEmitter<T_topic, T_msg>;

public:
  virtual ~EventBus();

  void connect(IEventBusKitConnection<T_topic, T_msg> *connection);
  void disconnect(IEventBusKitConnection<T_topic, T_msg> *connection);

  void connect(EventEmitter<T_topic, T_msg> *emitter);
  void disconnect(EventEmitter<T_topic, T_msg> *emitter);

  void connect(EventListener<T_topic, T_msg> *listener);
  void disconnect(EventListener<T_topic, T_msg> *listener);

  void emitEvent(T_topic topic, T_msg msg);
  virtual uint8_t execute() override;

private:
  ClearQueue<EventListener<T_topic, T_msg> *> mListeners; // TODO: need real set; std::set is broken into esp xtensa sdk
  ClearQueue<EventEmitter<T_topic, T_msg> *> mEmitters;
  ClearQueue<std::pair<T_topic, T_msg>> mEvents;
};

using CoreEventBus = EventBus<unsigned int, int>;
} // namespace uniot
