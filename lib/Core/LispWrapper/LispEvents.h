/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2024 Uniot <contact@uniot.io>
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

namespace uniot::events::lisp {
/**
 * @brief Communication channels for data exchange between Lisp and the host application
 */
enum Channel {
  OUT_LISP = FOURCC(lout),      ///< Channel for standard Lisp output
  OUT_LISP_LOG = FOURCC(llog),  ///< Channel for Lisp log messages
  OUT_LISP_ERR = FOURCC(lerr),  ///< Channel for Lisp error messages
  OUT_EVENT = FOURCC(evou),     ///< Channel for outgoing events from Lisp to the application
  IN_EVENT = FOURCC(evin),      ///< Channel for incoming events from the application to Lisp
};

/**
 * @brief Topics for event-based communication
 */
enum Topic {
  OUT_LISP_MSG = FOURCC(lisp),      ///< Topic for Lisp output messages
  OUT_LISP_REQUEST = FOURCC(lspr),  ///< Topic for Lisp requests to the application
  OUT_LISP_EVENT = FOURCC(levo),    ///< Topic for outgoing events from Lisp
  IN_LISP_EVENT = FOURCC(levi)      ///< Topic for incoming events to Lisp
};

/**
 * @brief Message types used in event communication
 */
enum Msg {
  OUT_MSG_ADDED,       ///< Standard output message was added
  OUT_MSG_LOG,         ///< Log message was added
  OUT_MSG_ERROR,       ///< Error message was added
  OUT_REFRESH_EVENTS,  ///< Request to refresh the event queue
  OUT_NEW_EVENT,       ///< New outgoing event
  IN_NEW_EVENT         ///< New incoming event
};

}  // namespace uniot::events::lisp
