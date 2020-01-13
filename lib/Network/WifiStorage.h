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

#include <EEPROM.h>

struct WifiArgs {
  String ssid;
  String pass;

  bool isValid() {
    return ssid.length() + pass.length();
  }
};

class WifiStorage
{
public:
  WifiArgs* getWifiArgs() {
    return &mWifiArgs;
  }

  void store() {
    EEPROM.begin(2 * STRING_SIZE);
    _write(0, mWifiArgs.ssid);
    _write(1, mWifiArgs.pass);
    EEPROM.end();
  }

  void restore() {
    EEPROM.begin(2 * STRING_SIZE);
    mWifiArgs.ssid = _read(0);
    mWifiArgs.pass = _read(1);
    EEPROM.end();
  }

  void clean() {
    EEPROM.begin(2 * STRING_SIZE);
    uint8_t* data = EEPROM.getDataPtr();
    memset(data, 0, 2 * STRING_SIZE);
    EEPROM.end();
  }

private:

  bool _write(uint8_t index, String str) {
    uint8_t* data = EEPROM.getDataPtr();
    if(data && str.length() < STRING_SIZE - 1) {
      // '-1' in if guarantees place for the '\0' character
      memcpy(data + index * STRING_SIZE, (const uint8_t*) str.c_str(), str.length());
      memset(data + index * STRING_SIZE + str.length(), 0, STRING_SIZE - str.length());
      return true;
    }
    return false;
  }

  String _read(uint8_t index) {
    if(!EEPROM.read((index + 1) * STRING_SIZE - 1)) {
      // String in storage must be terminated by the '\0' character
      // also EEPROMClass::read(int address) returns 0 if internal error occurs
      // EEPROM.getDataPtr() marks data as dirty and EEPROM commits at closing
      uint8_t* data = EEPROM.getDataPtr();
      if(data) { 
        return String((const char*)(data + index * STRING_SIZE));
      }
    }
    return String();
  }

//TODO: offset
  const uint8_t STRING_SIZE = 64;
  WifiArgs mWifiArgs;
};