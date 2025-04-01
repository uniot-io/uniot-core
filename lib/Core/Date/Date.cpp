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

#include <time.h>

/**
 * @brief Defines the minimal delay between SNTP updates
 * @ingroup date-time-main
 *
 * This function overrides the default SNTP update delay as defined in RFC.
 * While the RFC recommends not less than 15 seconds, this implementation
 * sets a much more conservative 10-minute interval to reduce network traffic.
 *
 * @retval uint32_t Delay in milliseconds (10 minutes)
 */
uint32_t sntp_update_delay_MS_rfc_not_less_than_15000() {
  return 10 * 60 * 1000UL;  // 10 minutes
}
