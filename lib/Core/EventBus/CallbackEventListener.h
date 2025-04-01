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

#include <functional>

#include "EventListener.h"

namespace uniot {
/**
 * @brief An event listener implementation that uses a callback function.
 * @defgroup event_bus_callback_listener Callback Listener
 * @ingroup event_bus_listener
 *
 * This class provides a way to handle events via callback functions instead of
 * subclassing EventListener directly. It wraps a callback function and calls it
 * when an event is received.
 *
 * @tparam T_topic The type used for event topics (typically an enum or integer).
 * @tparam T_msg The type used for event messages/payloads.
 * @tparam T_data The type used for storing additional event data.
 * @{
 */
template <class T_topic, class T_msg, class T_data>
class CallbackEventListener : public EventListener<T_topic, T_msg, T_data> {
 public:
  /**
   * @brief Callback function type for event handling.
   *
   * This function will be called when an event is received with the topic and message as parameters.
   */
  using EventListenerCallback = std::function<void(T_topic, T_msg)>;

  /**
   * @brief Constructs a CallbackEventListener with the given callback function.
   *
   * @param callback The function to call when an event is received.
   */
  CallbackEventListener(EventListenerCallback callback);

  /**
   * @brief Handler called when an event is received.
   *
   * This overrides the base EventListener method and invokes the stored callback.
   *
   * @param topic The topic of the received event.
   * @param msg The message/payload of the received event.
   */
  virtual void onEventReceived(T_topic topic, T_msg msg);

 private:
  EventListenerCallback mCallback; ///< The callback function to invoke for events
};

/**
 * @brief Convenience alias for the most common usage of CallbackEventListener.
 *
 * Uses unsigned int for topics, int for messages, and Bytes for data storage.
 */
using CoreCallbackEventListener = CallbackEventListener<unsigned int, int, Bytes>;
/** @} */
}  // namespace uniot
