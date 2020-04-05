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

#include <Arduino.h>

#define UNIOT_PIN_MAP_DIGITAL_INPUT  0
#define UNIOT_PIN_MAP_DIGITAL_OUTPUT 1
#define UNIOT_PIN_MAP_ANALOG_INPUT   2
#define UNIOT_PIN_MAP_ANALOG_OUTPUT  3

namespace uniot
{

class PinMap
{
public:
  PinMap() : mpMap(nullptr)
  {
    mpMap = new unsigned char *[MAP_COUNT];
    for (int i = 0; i < MAP_COUNT; i++)
    {
      mpMap[i] = nullptr;
      mMapLength[i] = 0;
    }
  }

  ~PinMap()
  {
    for (int i = 0; i < MAP_COUNT; ++i)
      _invalidateMap(i);

    delete[] mpMap;
    mpMap = nullptr;
  }

  void setDigitalInput(int count, ...)
  {
    va_list args;
    va_start(args, count);
    _resetMap(UNIOT_PIN_MAP_DIGITAL_INPUT, count, args, INPUT);
    va_end(args); 
  }

  void setDigitalOutput(int count, ...)
  {
    va_list args;
    va_start(args, count);
    _resetMap(UNIOT_PIN_MAP_DIGITAL_OUTPUT, count, args, OUTPUT);
    va_end(args);
  }

  void setAnalogInput(int count, ...)
  {
    va_list args;
    va_start(args, count);
    _resetMap(UNIOT_PIN_MAP_ANALOG_INPUT, count, args, INPUT);
    va_end(args);
  }

  void setAnalogOutput(int count, ...)
  {
    va_list args;
    va_start(args, count);
    _resetMap(UNIOT_PIN_MAP_ANALOG_OUTPUT, count, args, OUTPUT);
    va_end(args);
  }

  unsigned char getDigitalInput(int idx) const
  {
    return mpMap[UNIOT_PIN_MAP_DIGITAL_INPUT][idx];
  }

  unsigned char getDigitalOutput(int idx) const
  {
    return mpMap[UNIOT_PIN_MAP_DIGITAL_OUTPUT][idx];
  }

  unsigned char getAnalogInput(int idx) const
  {
    return mpMap[UNIOT_PIN_MAP_ANALOG_INPUT][idx];
  }

  unsigned char getAnalogOutput(int idx) const
  {
    return mpMap[UNIOT_PIN_MAP_ANALOG_OUTPUT][idx];
  }

  int getDigitalInputLength() const
  {
    return mMapLength[UNIOT_PIN_MAP_DIGITAL_INPUT];
  }

  int getDigitalOutputLength() const
  {
    return mMapLength[UNIOT_PIN_MAP_DIGITAL_OUTPUT];
  }

  int getAnalogInputLength() const
  {
    return mMapLength[UNIOT_PIN_MAP_ANALOG_INPUT];
  }

  int getAnalogOutputLength() const
  {
    return mMapLength[UNIOT_PIN_MAP_ANALOG_OUTPUT];
  }

private:
  void _resetMap(unsigned char idx, int count, va_list args, unsigned char mode)
  {
    _invalidateMap(idx, count);
    _copyToMap(idx, count, args);
    _setPinMode(idx, count, mode);
  }

  void _copyToMap(unsigned char idx, int count, va_list args)
  {
    for (int i = 0; i < count; i++)
    {
      auto pin = va_arg(args, int);
      mpMap[idx][i] = pin;
    }
  }

  void _setPinMode(unsigned char idx, int count, unsigned char mode)
  {
    for (int i = 0; i < count; i++)
      pinMode(mpMap[idx][i], mode);
  }

  void _invalidateMap(unsigned char idx, int count = 0)
  {
    if (idx >= MAP_COUNT)
      return;

    mMapLength[idx] = count;

    if (mpMap[idx])
    {
      delete mpMap[idx];
      mpMap[idx] = nullptr;
    }

    if (count > 0)
      mpMap[idx] = new unsigned char[count];
  }

  static const int MAP_COUNT = 4;
  int mMapLength[MAP_COUNT];
  unsigned char **mpMap;
};

static PinMap UniotPinMap;

} // namespace uniot
