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

#pragma once

#include <CBORStorage.h>
#include <EEPROM.h>

namespace uniot {
class WifiStorage : public CBORStorage {
 public:
  WifiStorage() : CBORStorage("wifi.cbor") {
  }

  const String& getSsid() const {
    return mSsid;
  }

  const String& getPassword() const {
    return mPassword;
  }

  void setCredentials(const String& ssid, const String& password) {
    mSsid = ssid;
    mPassword = password;
  }

  bool isCredentialsValid() {
    return mSsid.length() && mPassword.length();
  }

  virtual bool store() override {
    object().put("ssid", mSsid.c_str());
    object().put("pass", mPassword.c_str());
    return CBORStorage::store();
  }

  virtual bool restore() override {
    if (CBORStorage::restore()) {
      mSsid = object().getString("ssid");
      mPassword = object().getString("pass");
      return true;
    }
    return false;
  }

  virtual bool clean() override {
    mSsid = "";
    mPassword = "";
    return CBORStorage::clean();
  }

 private:
  String mSsid;
  String mPassword;
};
}  // namespace uniot
