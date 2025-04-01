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

#include <Bytes.h>
#include <ClearQueue.h>
#include <IterableQueue.h>

#include "EventEmitter.h"

namespace uniot {
/**
 * @brief Listener for EventBus events that can subscribe to specific topics
 * @defgroup event_bus_listener Listener
 * @ingroup event_bus_entity
 *
 * EventListener is a base class for objects that need to listen to events on the event bus.
 * It extends EventEmitter to also allow for receiving events through the event system.
 *
 * @tparam T_topic Type used for topic identification (e.g. unsigned int)
 * @tparam T_msg Type used for message payload (e.g. int)
 * @tparam T_data Type used for additional data (e.g. Bytes)
 * @{
 */
template <class T_topic, class T_msg, class T_data>
class EventListener : public EventEmitter<T_topic, T_msg, T_data> {
 public:
  /**
   * @brief Virtual destructor
   */
  virtual ~EventListener() = default;

  /**
   * @brief Get the type ID of this class for RTTI
   * @retval type_id The unique type identifier for this EventListener specialization
   */
  virtual type_id getTypeId() const override {
    return Type::getTypeId<EventListener<T_topic, T_msg, T_data>>();
  }

  /**
   * @brief Subscribe to events of a specific topic
   * @param topic The topic to listen to
   * @retval EventListener* Pointer to this instance for method chaining
   */
  EventListener *listenToEvent(T_topic topic);

  /**
   * @brief Unsubscribe from events of a specific topic
   * @param topic The topic to stop listening to
   * @retval EventListener* Pointer to this instance for method chaining
   */
  EventListener *stopListeningToEvent(T_topic topic);

  /**
   * @brief Check if this listener is subscribed to a specific topic
   * @param topic The topic to check
   * @retval true The listener is subscribed to the topic
   * @retval false The listener is not subscribed to the topic
   */
  bool isListeningToEvent(T_topic topic);

  /**
   * @brief Handler called when an event is received
   *
   * This method must be implemented by derived classes to handle
   * incoming events for topics that this listener has subscribed to.
   *
   * @param topic The topic of the received event
   * @param msg The message payload of the received event
   */
  virtual void onEventReceived(T_topic topic, T_msg msg) = 0;

 private:
  /** @brief Queue storing topics this listener is subscribed to */
  ClearQueue<T_topic> mTopics;
};

/**
 * @brief Type alias for the common EventListener configuration used in the core system
 *
 * Uses unsigned int for topics, int for messages, and Bytes for data
 */
using CoreEventListener = EventListener<unsigned int, int, Bytes>;
/** @} */
}  // namespace uniot
