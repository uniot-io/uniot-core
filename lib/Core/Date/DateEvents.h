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
 * @file DateEvents.h
 * @brief Date and time event definitions for the Uniot event system
 * @defgroup date_events Date Events
 * @ingroup date
 * @{
 *
 * This file defines the event topics and messages used throughout the Uniot
 * date and time subsystem. These events enable communication between time
 * synchronization services, schedulers, and other system components that
 * need to respond to time-related state changes.
 *
 * The event system uses FOURCC (Four Character Code) identifiers for efficient
 * event routing and minimal memory footprint. Date events are organized into:
 * - Topics: Event categories (e.g., time synchronization events)
 * - Messages: Specific event types within topics (e.g., synced, failed)
 *
 * Example usage:
 * @code
 * // Emit a time synchronization success event
 * eventEmitter.emitEvent(events::date::Topic::TIME,
 *                       events::date::Msg::SYNCED);
 *
 * // Listen for time synchronization events
 * eventListener.listenToEvent(events::date::Topic::TIME);
 * @endcode
 */

namespace uniot::events::date {

/**
 * @brief Date and time event topics
 *
 * Defines the main categories of date and time events that can be emitted
 * throughout the system. Topics group related events together and use
 * FOURCC identifiers for efficient event handling and type safety.
 */
enum Topic {
  TIME = FOURCC(date)  ///< Time synchronization and date-related operations
};

/**
 * @brief Date and time event messages
 *
 * Defines specific event messages that can be sent within date topics.
 * These messages represent different states and outcomes of time-related
 * operations, particularly time synchronization and clock management.
 */
enum Msg {
  SYNCED = 0  ///< System time has been successfully synchronized with time source
};

}  // namespace uniot::events::date

/** @} */
