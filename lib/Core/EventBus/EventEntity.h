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

#pragma once

#include <IterableQueue.h>
#include <Logger.h>

namespace uniot {
template <class T_topic, class T_msg, class T_data>
class EventBus;

template <class T_topic, class T_msg, class T_data>
class EventEntity {
 public:
  using DataChannelCallback = std::function<void(unsigned int, bool, T_data)>;

  virtual ~EventEntity() = default;

  bool sendDataToChannel(T_topic channel, T_data data) {
    auto sentSomewhere = false;
    this->mEventBusQueue.forEach([&](EventBus<T_topic, T_msg, T_data> *eventBus) {
      auto sent = eventBus->sendDataToChannel(channel, data);
      sentSomewhere |= sent;
      yield();
    });
    return sentSomewhere;
  }

  void receiveDataFromChannel(T_topic channel, DataChannelCallback callback) {
    this->mEventBusQueue.forEach([&](EventBus<T_topic, T_msg, T_data> *eventBus) {
      if (callback) {
        auto empty = eventBus->isDataChannelEmpty(channel);
        auto data = eventBus->receiveDataFromChannel(channel);
        callback(eventBus->getId(), empty, data);
      }
      yield();
    });
  }

 protected:
  bool connectUnique(EventBus<T_topic, T_msg, T_data> *eventBus) {
    mEventBusQueue.begin();
    while (!mEventBusQueue.isEnd()) {
      auto connectedEventBus = mEventBusQueue.current();
      if (connectedEventBus->getId() == eventBus->getId()) {
        UNIOT_LOG_INFO("EventBus with id %d already connected", eventBus->getId());
        return false;
      }
      mEventBusQueue.next();
    }
    return mEventBusQueue.pushUnique(eventBus);
  }

  IterableQueue<EventBus<T_topic, T_msg, T_data> *> mEventBusQueue;
};
}  // namespace uniot
