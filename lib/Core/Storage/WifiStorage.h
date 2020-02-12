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

#include <EEPROM.h>
#include <CBORStorage.h>

struct WifiArgs
{
  String ssid;
  String pass;

  bool isValid()
  {
    return ssid.length() + pass.length();
  }
};

namespace uniot
{
class WifiStorage : public CBORStorage
{
public:
  WifiStorage() : CBORStorage("wifi.cbor")
  {
  }

  WifiArgs *getWifiArgs()
  {
    return &mWifiArgs;
  }

  void store()
  {
    object().put("ssid", mWifiArgs.ssid.c_str());
    object().put("pass", mWifiArgs.pass.c_str());
    CBORStorage::store();
  }

  void restore()
  {
    CBORStorage::restore();
    mWifiArgs.ssid = object().getString("ssid");
    mWifiArgs.pass = object().getString("pass");
  }

  void clean()
  {
    CBORStorage::clean();
    mWifiArgs.ssid = "";
    mWifiArgs.pass = "";
  }

private:
  WifiArgs mWifiArgs;
};
} // namespace uniot
