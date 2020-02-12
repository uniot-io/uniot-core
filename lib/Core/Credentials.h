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
#include <user_interface.h>

#include <CBORStorage.h>

namespace uniot
{
class Credentials : public CBORStorage
{
public:
  Credentials() : CBORStorage("credentials.cbor")
  {
    CBORStorage::restore();

    mCreatorId = UNIOT_CREATOR_ID;
    mOwnerId = object().getString("account");
    mDeviceId = _calcDeviceId();
  }

  bool store()
  {
    object().put("account", mOwnerId.c_str());
    return CBORStorage::store();
  }

  void setOwnerId(const String &id)
  {
    mOwnerId = id;
  }

  String getOwnerId() const
  {
    return mOwnerId;
  }

  String getCreatorId() const
  {
    return mCreatorId;
  }

  String getDeviceId() const
  {
    return mDeviceId;
  }

  uint32_t getShortDeviceId() const
  {
    return ESP.getChipId();
  }

private:
  String _calcDeviceId()
  {
    uint8_t mac[6];
    char macStr[13] = {0};
    wifi_get_macaddr(STATION_IF, mac);

    for (uint8_t i = 0; i < 6; i++)
      sprintf(macStr + i * 2, "%02x", mac[i]);

    return String(macStr);
  }

  String mOwnerId;
  String mCreatorId;
  String mDeviceId;
};
} // namespace uniot
