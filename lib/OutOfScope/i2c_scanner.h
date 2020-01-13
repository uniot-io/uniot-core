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

#include <Arduino.h>
#include <Wire.h>

class I2CScanner{
public:
  static void scan(TwoWire* twoWire, HardwareSerial* serial){
    serial->println("I2CScanner: begin");
    for(byte address = 1; address < 127; ++address) 
    {
      twoWire->beginTransmission(address);
      switch(twoWire->endTransmission()){
        case 0:
        serial->print("I2CScanner: found on 0x");
        serial->print(address < 16 ? "0" : "");
        serial->println(address, HEX);
        break;
        case 4:
        serial->print("I2CScanner: error on 0x");
        serial->print(address < 16 ? "0" : "");
        serial->println(address, HEX);
        break;
        default:
        break;
      }
    }
    serial->println("I2CScanner: end");
  }
};
