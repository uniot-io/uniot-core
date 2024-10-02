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

/*
 * WiFi Transmission Power Patch
 *
 * ENABLE_LOWER_WIFI_TX_POWER:
 *   Define this macro to enable lowered WiFi transmission power settings.
 *
 *   Some ESP32-C3 modules may require lower Tx power to ensure stable
 *   WiFi connections due to poor antenna design or power supply limitations.
 *   High Tx power can cause unstable WiFi connections or failure to connect to networks.
 *
 * WIFI_TX_POWER_LEVEL:
 *   Defines the desired WiFi transmission power level.
 *
 *   Available options (depending on the ESP32 library):
 *     - WIFI_POWER_5dBm
 *     - WIFI_POWER_8_5dBm
 *     - WIFI_POWER_11dBm
 *     - WIFI_POWER_13dBm
 *
 *   Lowering the Tx power can mitigate issues related to signal interference
 *   and power instability on certain hardware configurations.
 */

// #define ENABLE_LOWER_WIFI_TX_POWER
#ifdef ENABLE_LOWER_WIFI_TX_POWER

#ifndef WIFI_TX_POWER_LEVEL
#define WIFI_TX_POWER_LEVEL WIFI_POWER_8_5dBm
#endif

#endif
