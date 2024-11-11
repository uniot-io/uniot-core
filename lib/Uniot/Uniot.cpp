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
#if defined(ESP8266)
#include <CrashStorage.h>
#endif
#include <Uniot.h>

UniotCore Uniot;

#if defined(ESP8266)
extern "C" void custom_crash_callback(struct rst_info *resetInfo, uint32_t stackStart, uint32_t stackEnd) {
  uniot::uniotCrashCallback(resetInfo, stackStart, stackEnd);
}
#endif
