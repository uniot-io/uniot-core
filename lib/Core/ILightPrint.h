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

namespace uniot {

class ILightPrint {
 public:
  virtual ~ILightPrint() {}
  virtual void println(const char* str) = 0;
  virtual void printlnPGM(const char* PSTR_str) = 0;
};

class SerialLightPrinter : public ILightPrint {
 public:
  virtual void println(const char* str) override {
    Serial.println(str);
  }
  virtual void printlnPGM(const char* PSTR_str) override {
    Serial.println(PSTR_str);
  }
};

}  // namespace uniot
