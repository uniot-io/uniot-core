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
 * @file IEventBusConnectionKit.h
 * @brief Defines the interface for components that can connect to an EventBus
 *
 * This file contains the interface definition for components that need to
 * register and unregister with an EventBus. It provides a standardized way
 * for components to interact with the event bus system.
 */

#pragma once

#include "EventBus.h"

namespace uniot {
template <class T_topic, class T_msg, class T_data>
class EventBus;

/**
 * @brief Interface for components that need to connect to an EventBus
 * @defgroup event_bus_connectionkit Connection Kit
 * @ingroup event_bus_main
 *
 * This interface defines the contract that components must implement
 * to be able to register and unregister with an EventBus. It provides
 * the necessary methods for establishing and terminating the connection
 * between a component and the event bus.
 *
 * @tparam T_topic Type used for the topic identifiers
 * @tparam T_msg Type used for message identifiers
 * @tparam T_data Type used for the payload data
 * @{
 */
template <class T_topic, class T_msg, class T_data>
class IEventBusConnectionKit {
 public:
  /**
   * @brief Virtual destructor to ensure proper cleanup of derived classes
   */
  virtual ~IEventBusConnectionKit() {}

  /**
   * @brief Registers this component with the specified event bus
   *
   * Implementing classes should use this method to subscribe to
   * relevant topics and set up message handlers.
   *
   * @param eventBus Reference to the event bus to register with
   */
  virtual void registerWithBus(EventBus<T_topic, T_msg, T_data> &eventBus) = 0;

  /**
   * @brief Unregisters this component from the specified event bus
   *
   * Implementing classes should use this method to unsubscribe from
   * topics and clean up any resources associated with the event bus.
   *
   * @param eventBus Reference to the event bus to unregister from
   */
  virtual void unregisterFromBus(EventBus<T_topic, T_msg, T_data> &eventBus) = 0;
};

/**
 * @brief A specific instance of IEventBusConnectionKit for core system events
 *
 * This alias defines a common interface for components that need to connect
 * to the core event bus, using unsigned int for topics, int for messages,
 * and Bytes for payload data.
 */
using ICoreEventBusConnectionKit = IEventBusConnectionKit<unsigned int, int, Bytes>;
/** @} */
}  // namespace uniot
