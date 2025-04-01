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

/**
 * @file Patches.h
 * @defgroup patches Patches
 * @brief Configuration patches for hardware-specific optimizations
 * @{
 *
 * This file contains configuration macros that modify the behavior of the
 * Uniot framework to accommodate hardware-specific requirements or limitations.
 * Each patch section contains detailed documentation about its purpose, usage, and
 * available configuration options.
 */

/**
 * @defgroup wifi_tx_power WiFi Transmission Power Patch
 * @brief Configures WiFi transmission power settings for ESP32 devices
 * @{
 *
 * @details
 * This patch allows for adjusting the WiFi transmission power levels, which can
 * be necessary for certain ESP32 modules with design limitations or to optimize
 * power consumption and connection stability.
 *
 * Some ESP32-C3 modules may require lower Tx power to ensure stable WiFi connections
 * due to:
 * - Poor antenna design
 * - Power supply limitations
 * - Signal interference concerns
 *
 * High transmission power can lead to:
 * - Unstable WiFi connections
 * - Failed network connections
 * - Excessive power consumption
 * - Device overheating
 *
 * @section wifi_tx_usage Usage
 * Define ENABLE_LOWER_WIFI_TX_POWER in your build configuration to activate
 * this patch. Optionally, define WIFI_TX_POWER_LEVEL to set a specific power level.
 *
 * @section wifi_tx_config Configuration Options
 * @par WIFI_TX_POWER_LEVEL
 * Available options (ESP32 SDK dependent):
 * - WIFI_POWER_5dBm    (Lowest power, ~5dBm)
 * - WIFI_POWER_8_5dBm  (Low power, ~8.5dBm) [Default when patch is enabled]
 * - WIFI_POWER_11dBm   (Medium power, ~11dBm)
 * - WIFI_POWER_13dBm   (Medium-high power, ~13dBm)
 */
#ifdef ENABLE_LOWER_WIFI_TX_POWER

#ifndef WIFI_TX_POWER_LEVEL
#define WIFI_TX_POWER_LEVEL WIFI_POWER_8_5dBm
#endif

#endif
/** @} */ /* End of wifi_tx_power group */

/** @} */ /* End of patches group */
