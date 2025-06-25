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

namespace uniot::events::mqtt {
/**
 * @enum Topic
 * @brief Event topics this class can emit
 */
enum Topic {
  CONNECTION = FOURCC(mqtt) /**< MQTT connection topic (value from FOURCC("mqtt")) */
};

/**
 * @enum Msg
 * @brief Message types for the CONNECTION topic
 */
enum Msg {
  FAILED = 0, /**< Connection failed */
  SUCCESS = 1 /**< Connection successful */
};

}  // namespace uniot::events::lisp
