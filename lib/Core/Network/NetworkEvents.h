/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2025 Uniot <contact@uniot.io>
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

#include <Common.h>

/**
 * @file NetworkEvents.h
 * @brief Network event definitions for the Uniot event system
 * @defgroup network_events Network Events
 * @ingroup network
 * @{
 *
 * This file defines the event channels, topics, and messages used throughout
 * the Uniot network subsystem. These events enable communication between
 * different components of the system such as network schedulers, LED indicators,
 * and application logic.
 *
 * The event system uses FOURCC (Four Character Code) identifiers for efficient
 * event routing and minimal memory footprint. Events are organized into:
 * - Channels: Data transmission pathways (e.g., SSID output)
 * - Topics: Event categories (e.g., connection events, LED control)
 * - Messages: Specific event types within topics (e.g., success, failure)
 *
 * Example usage:
 * @code
 * // Emit a connection success event
 * eventEmitter.emitEvent(events::network::Topic::CONNECTION,
 *                       events::network::Msg::SUCCESS);
 *
 * // Send SSID data via channel
 * eventEmitter.sendDataToChannel(events::network::Channel::OUT_SSID,
 *                               Bytes(ssidString));
 * @endcode
 */

namespace uniot::events::network {

/**
 * @brief Network-related data channels
 *
 * Defines communication channels for transmitting network-related data
 * between system components. Channels use FOURCC identifiers for efficient
 * routing and type safety.
 */
enum Channel {
  OUT_SSID = FOURCC(ssid)  ///< Channel for broadcasting current SSID information
};

/**
 * @brief Network event topics
 *
 * Defines the main categories of network events that can be emitted
 * throughout the system. Topics group related events together and use
 * FOURCC identifiers for efficient event handling.
 */
enum Topic {
  CONNECTION = FOURCC(netw),      ///< WiFi connection state changes and operations
  WIFI_STATUS_LED = FOURCC(nled)  ///< LED status indicators for network state
};

/**
 * @brief Network event messages
 *
 * Defines specific event messages that can be sent within network topics.
 * These messages represent different states and outcomes of network operations.
 */
enum Msg {
  FAILED = 0,     ///< Network operation failed (connection, scan, etc.)
  SUCCESS,        ///< Network operation completed successfully
  CONNECTING,     ///< Currently attempting to connect to network
  DISCONNECTING,  ///< Currently disconnecting from network
  DISCONNECTED,   ///< Network connection has been lost or terminated
  ACCESS_POINT,   ///< Device is operating in access point mode
  AVAILABLE       ///< Configured network is available for connection
};

}  // namespace uniot::events::network

/** @} */
