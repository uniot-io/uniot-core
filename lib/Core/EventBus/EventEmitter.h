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
#include <IterableQueue.h>

#include "EventEntity.h"

namespace uniot {
/**
 * @brief A template class for emitting events to registered event buses.
 * @defgroup event_bus_emitter Emitter
 * @ingroup event_bus_entity
 *
 * The EventEmitter allows components to publish events to multiple event buses
 * through a common interface. It serves as a publisher in the observer pattern.
 *
 * @tparam T_topic The data type for event topics (usually an enum or integer)
 * @tparam T_msg The data type for event messages/codes (usually an enum or integer)
 * @tparam T_data The data type for additional event payload data
 * @{
 */
template <class T_topic, class T_msg, class T_data>
class EventEmitter : public EventEntity<T_topic, T_msg, T_data> {
 public:
  /**
   * @brief Virtual destructor to ensure proper cleanup of derived classes
   */
  virtual ~EventEmitter() = default;

  /**
   * @brief Returns the type identifier for this EventEmitter
   *
   * Implements the base class virtual method to enable type identification
   * via the Type system.
   *
   * @retval type_id The unique type identifier for this EventEmitter specialization
   */
  virtual type_id getTypeId() const override {
    return Type::getTypeId<EventEmitter<T_topic, T_msg, T_data>>();
  }

  /**
   * @brief Emits an event to all registered event buses
   *
   * Broadcasts the specified event (topic and message) to all event buses
   * that have been registered with this emitter.
   *
   * @param topic The topic of the event
   * @param msg The message/code of the event
   */
  void emitEvent(T_topic topic, T_msg msg);
};

/**
 * @brief A specialized EventEmitter for core system events
 *
 * This type alias defines the standard EventEmitter used throughout the core system,
 * with unsigned int as topic type, int as message type, and Bytes as data container.
 */
using CoreEventEmitter = EventEmitter<unsigned int, int, Bytes>;
/** @} */
}  // namespace uniot
