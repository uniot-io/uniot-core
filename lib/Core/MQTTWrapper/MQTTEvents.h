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
 * @file MQTTEvents.h
 * @brief MQTT event definitions for the Uniot event system
 * @defgroup mqtt_events MQTT Events
 * @ingroup mqtt
 * @{
 *
 * This file defines the event topics and messages used throughout the Uniot
 * MQTT subsystem. These events enable communication between MQTT clients,
 * connection managers, and other system components that need to respond to
 * MQTT connectivity changes.
 *
 * The event system uses FOURCC (Four Character Code) identifiers for efficient
 * event routing and minimal memory footprint. MQTT events are organized into:
 * - Topics: Event categories (e.g., connection events)
 * - Messages: Specific event types within topics (e.g., success, failure)
 *
 * Example usage:
 * @code
 * // Emit an MQTT connection success event
 * eventEmitter.emitEvent(events::mqtt::Topic::CONNECTION,
 *                       events::mqtt::Msg::SUCCESS);
 *
 * // Listen for MQTT connection events
 * eventListener.listenToEvent(events::mqtt::Topic::CONNECTION);
 * @endcode
 */

namespace uniot::events::mqtt {

/**
 * @brief MQTT event topics
 *
 * Defines the main categories of MQTT events that can be emitted throughout
 * the system. Topics group related events together and use FOURCC identifiers
 * for efficient event handling and type safety.
 */
enum Topic {
  CONNECTION = FOURCC(mqtt)  ///< MQTT connection state changes and operations
};

/**
 * @brief MQTT event messages
 *
 * Defines specific event messages that can be sent within MQTT topics.
 * These messages represent different states and outcomes of MQTT operations,
 * particularly connection establishment and failure scenarios.
 */
enum Msg {
  FAILED = 0,  ///< MQTT connection failed or was lost
  SUCCESS = 1  ///< MQTT connection established successfully
};

}  // namespace uniot::events::mqtt

/** @} */
