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

/** @cond */
/**
 * DO NOT DELETE THE "event_bus" GROUP DEFINITION BELOW.
 * Used to create the Internal Comunication topic in the documentation.
 * If you want to delete this file, please paste the group definition
 * into another utility and delete this one.
 */
/** @endcond */

/**
 * @defgroup event_bus Internal Communication
 * @brief Event-driven communication module for component interactions.
 *
 * The internal communication module provides a robust infrastructure for
 * decoupled message exchange between system components. It implements
 * the publish-subscribe pattern allowing components to communicate without
 * direct dependencies. The module supports event broadcasting, targeted
 * message delivery, and asynchronous data channels for efficient
 * information exchange across the system.
 */

#pragma once

#include <Bytes.h>
#include <ClearQueue.h>
#include <DataChannels.h>
#include <IExecutor.h>

#include "IEventBusConnectionKit.h"

namespace uniot {
template <class T_topic, class T_msg, class T_data>
class EventEntity;

template <class T_topic, class T_msg, class T_data>
class EventEmitter;

template <class T_topic, class T_msg, class T_data>
class EventListener;

/**
 * @brief The EventBus is a central communication hub for event-driven architecture.
 * @defgroup event_bus_main Event Bus
 * @ingroup event_bus
 * @{
 *
 * EventBus implements a publish-subscribe pattern allowing decoupled components to
 * communicate through events. It manages event subscriptions, broadcasts events to
 * interested listeners, and provides data channel functionality for asynchronous
 * data exchange between components.
 *
 * @tparam T_topic Type used for event topics (typically unsigned int)
 * @tparam T_msg Type used for event messages (typically int)
 * @tparam T_data Type used for data channel payloads (typically Bytes)
 */
template <class T_topic, class T_msg, class T_data>
class EventBus : public uniot::IExecutor {
  friend class EventEntity<T_topic, T_msg, T_data>;

 public:
  /**
   * @brief Constructs an EventBus with a unique identifier
   * @param id Unique identifier for this EventBus
   */
  EventBus(unsigned int id) : mId(id) {}

  /**
   * @brief Destroys the EventBus and cleans up registered entities
   */
  virtual ~EventBus();

  /**
   * @brief Gets the unique identifier of this EventBus
   * @retval id The identifier assigned to this EventBus
   */
  unsigned int getId() { return mId; }

  /**
   * @brief Registers a connection kit with this EventBus
   *
   * Allows external connection kits to register and interact with this EventBus.
   *
   * @param connection The connection kit to register
   */
  void registerKit(IEventBusConnectionKit<T_topic, T_msg, T_data> &connection);

  /**
   * @brief Unregisters a connection kit from this EventBus
   *
   * @param connection The connection kit to unregister
   */
  void unregisterKit(IEventBusConnectionKit<T_topic, T_msg, T_data> &connection);

  /**
   * @brief Registers an entity (emitter or listener) with this EventBus
   *
   * @param entity The entity to register
   * @retval true Registration was successful
   * @retval false Registration failed (e.g., entity already registered)
   */
  bool registerEntity(EventEntity<T_topic, T_msg, T_data> *entity);

  /**
   * @brief Unregisters an entity from this EventBus
   *
   * @param entity The entity to unregister
   */
  void unregisterEntity(EventEntity<T_topic, T_msg, T_data> *entity);

  /**
   * @brief Opens a data channel for a specific topic with a size limit
   *
   * Data channels allow for asynchronous data exchange between components.
   *
   * @param topic The topic identifier for the data channel
   * @param limit Maximum number of items the channel can hold
   * @retval true Channel was opened successfully
   * @retval false Channel is already open or failed to open
   */
  bool openDataChannel(T_topic topic, size_t limit);

  /**
   * @brief Closes a previously opened data channel
   *
   * @param topic The topic identifier of the channel to close
   * @retval true Channel was closed successfully
   * @retval false Channel is not open or already closed
   */
  bool closeDataChannel(T_topic topic);

  /**
   * @brief Sends data to a specific data channel
   *
   * @param topic The topic identifier of the target channel
   * @param data The data to send
   * @retval true Data was successfully sent to the channel
   * @retval false Channel is not open or full
   */
  bool sendDataToChannel(T_topic topic, T_data data);

  /**
   * @brief Receives data from a specific data channel
   *
   * @param topic The topic identifier of the channel to receive from
   * @retval data The received data
   */
  T_data receiveDataFromChannel(T_topic topic);

  /**
   * @brief Checks if a data channel is empty
   *
   * @param topic The topic identifier of the channel to check
   * @retval true Channel is empty
   * @retval false Channel contains data
   */
  bool isDataChannelEmpty(T_topic topic);

  /**
   * @brief Emits an event to all registered listeners
   *
   * Queues an event for processing during the next execute() call.
   *
   * @param topic The topic identifier of the event
   * @param msg The message payload of the event
   */
  void emitEvent(T_topic topic, T_msg msg);

  /**
   * @brief Processes all queued events
   *
   * Distributes all queued events to appropriate listeners. This method
   * should be called regularly from the main application loop.
   *
   * @param _ Unused parameter (inherited from IExecutor)
   */
  virtual void execute(short _) override;

 private:
 // TODO: need real set; std::set is broken into esp xtensa sdk
  ClearQueue<EventEntity<T_topic, T_msg, T_data> *> mEntities; // Stores registered entities (emitters and listeners)
  ClearQueue<Pair<T_topic, T_msg>> mEvents;  // Queue of pending events to be processed

  DataChannels<T_topic, T_data> mDataChannels;  // Storage for data channels

  unsigned int mId;  // Unique identifier for this event bus
};

/**
 * @brief Standard EventBus configuration used throughout the core system
 *
 * Uses unsigned int for topics, int for messages, and Bytes for data payloads
 */
using CoreEventBus = EventBus<unsigned int, int, Bytes>;
/** @} */
}  // namespace uniot
