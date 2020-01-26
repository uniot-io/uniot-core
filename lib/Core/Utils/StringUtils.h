/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2020 Uniot <info.uniot@gmail.com>
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

#include <stdlib.h>
#include <string.h>

class StringUtils {
public:
  inline static unsigned char hexToChar(char ch) {
    ch = toupper(ch);
    return ch < 'A' ? (ch - '0') & 0x0F : (ch - 'A' + 0x0A);
  }

  static unsigned int hexStrToBytes(const char *src, unsigned char *dst, unsigned int len = -1) {
    unsigned char Hi, Lo;
    unsigned int i = 0;
    while ((Hi = *src++) && (Lo = *src++) && (i < len)) {
      dst[i++] = (hexToChar(Hi) << 4) | hexToChar(Lo);
    }
    return i;
  }

  static void bytesToHexStr(unsigned char *src, char *dst, unsigned int len = 0) {
    static const char lut[] = "0123456789abcdef";
    if ( ! len) {
      len = strlen((const char *)src);
    }
    for (size_t i = 0; i < len; ++i) {
      dst[i * 2] = lut[src[i] >> 4];
      dst[i * 2 + 1] = lut[src[i] & 15];
    }
  }
};