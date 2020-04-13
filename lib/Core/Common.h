/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2020 Uniot <contact@uniot.io>
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

#include <memory>
#include <utility>

template <int a, int b, int c, int d>
struct FourCC
{
  static const unsigned int Value = (((((d << 8) | c) << 8) | b) << 8) | a;
};

template <typename... Args>
inline void UNUSED(Args &&... args)
{
  (void)(sizeof...(args));
}

inline uint32_t CRC32(const void *data, size_t length, uint32_t crc = 0)
{
  const uint8_t *ldata = (const uint8_t *)data;

  crc = ~crc;
  while (length--)
  {
    crc ^= *ldata++;
    for (uint8_t k = 0; k < 8; k++)
      crc = crc & 1 ? (crc >> 1) ^ 0x82f63b78 : crc >> 1; // CRC-32C (iSCSI) polynomial in reversed bit order.
  }
  return ~crc;
}

#define COUNT_OF(arr) (sizeof(arr) / sizeof(arr[0]))
#define ARRAY_ELEMENT_SAFE(arr, index) ((arr)[(((index) < COUNT_OF(arr)) ? (index) : (COUNT_OF(arr) - 1))])
#define FOURCC(name) FourCC<ARRAY_ELEMENT_SAFE(#name, 0), ARRAY_ELEMENT_SAFE(#name, 1), ARRAY_ELEMENT_SAFE(#name, 2), ARRAY_ELEMENT_SAFE(#name, 3)>::Value

#define ALIAS_FUNCTION(high, low)                                               \
  template <typename... Args>                                                   \
  inline auto high(Args &&... args)->decltype(low(std::forward<Args>(args)...)) \
  {                                                                             \
    return low(std::forward<Args>(args)...);                                    \
  }

namespace uniot
{

template <typename T>
using UniquePointer = std::unique_ptr<T>;

template <typename T>
using SharedPointer = std::shared_ptr<T>;

template <typename T_First, typename T_Second>
using Pair = std::pair<T_First, T_Second>;

ALIAS_FUNCTION(MakePair, std::make_pair)

} // namespace uniot
