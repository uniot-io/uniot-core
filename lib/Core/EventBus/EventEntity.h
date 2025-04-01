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

/**
 * @file EventEntity.h
 * @defgroup event_bus_entity Event Entity
 * @ingroup event_bus
 * @brief Entity that can connect to and interact with EventBus instances
 *
 * Entity serves as a base class for components that need to communicate
 * through the event system. It can connect to multiple EventBus instances
 * and provides methods for sending and receiving data through channels.
 */

#pragma once

#include <IterableQueue.h>
#include <Logger.h>

#include "TypeId.h"

namespace uniot {
template <class T_topic, class T_msg, class T_data>
class EventBus;

/**
 * @brief Entity that can connect to and interact with EventBus instances
 * @ingroup event_bus_entity
 *
 * EventEntity serves as a base class for components that need to communicate
 * through the event system. It can connect to multiple EventBus instances
 * and provides methods for sending and receiving data through channels.
 *
 * @tparam T_topic Type used for channel identification (typically unsigned int)
 * @tparam T_msg Type used for message identification (typically int)
 * @tparam T_data Type of data being transmitted through channels (typically Bytes)
 * @{
 */
template <class T_topic, class T_msg, class T_data>
class EventEntity : public IWithType {
  friend class EventBus<T_topic, T_msg, T_data>;

 public:
  /**
   * @brief Callback type for handling data received from channels
   *
   * @param unsigned int EventBus ID
   * @param bool Whether the channel was empty before receiving
   * @param T_data The data received from the channel
   */
  using DataChannelCallback = std::function<void(unsigned int, bool, T_data)>;

  /**
   * @brief Destructor - disconnects from all connected EventBus instances
   *
   * Removes this entity from all connected EventBus instances to prevent
   * dangling references and ensure proper cleanup.
   */
  virtual ~EventEntity();

  /**
   * @brief Returns the type ID of this class
   *
   * Implementation of IWithType interface method.
   *
   * @retval type_id The unique type identifier for this class
   */
  virtual type_id getTypeId() const override {
    return Type::getTypeId<EventEntity<T_topic, T_msg, T_data>>();
  }

  /**
   * @brief Sends data to a specific channel on all connected EventBus instances
   *
   * Iterates through all connected EventBus instances and attempts to send
   * the provided data to the specified channel on each.
   *
   * @param channel The channel to send data to
   * @param data The data to be sent
   * @retval true Data was successfully sent to at least one EventBus
   * @retval false No EventBus was able to send the data (e.g., all channels were full)
   */
  bool sendDataToChannel(T_topic channel, T_data data) {
    auto sentSomewhere = false;
    this->mEventBusQueue.forEach([&](EventBus<T_topic, T_msg, T_data> *eventBus) {
      auto sent = eventBus->sendDataToChannel(channel, data);
      sentSomewhere |= sent;
      yield();
    });
    return sentSomewhere;
  }

  /**
   * @brief Receives data from a specific channel on all connected EventBus instances
   *
   * Iterates through all connected EventBus instances and retrieves data from
   * the specified channel on each, invoking the callback for each data item.
   *
   * @param channel The channel to receive data from
   * @param callback Function to be called for each data item received
   */
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
  /**
   * @brief Connects to an EventBus instance if not already connected
   *
   * Checks if the EventBus with the same ID is already connected.
   * If not, adds it to the queue of connected EventBus instances.
   *
   * @param eventBus Pointer to the EventBus to connect to
   * @retval true Connection was successful
   * @retval false Connection failed (e.g., EventBus already connected)
   */
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

  /**
   * @brief Queue of EventBus instances this entity is connected to
   *
   * This instantiation creates a concrete class for the most commonly used
   * template parameters: unsigned int for topics, int for messages, and Bytes for data.
   */
  IterableQueue<EventBus<T_topic, T_msg, T_data> *> mEventBusQueue;
};
/** @} */
}  // namespace uniot
