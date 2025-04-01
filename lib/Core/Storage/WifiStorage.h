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
/**
 * @brief Storage class for WiFi credentials
 * @defgroup fs_storage_wifi WiFi Storage
 * @ingroup fs_storage
 *
 * WifiStorage provides methods for saving, retrieving, and managing WiFi credentials
 * (SSID and password). It persists the data in CBOR format in a file named "wifi.cbor".
 * @{
 */
class WifiStorage : public CBORStorage {
 public:
  /**
   * @brief Construct a new WifiStorage object
   *
   * Initializes the storage with the filename "wifi.cbor"
   */
  WifiStorage() : CBORStorage("wifi.cbor") {
  }

  /**
   * @brief Get the stored WiFi SSID
   *
   * @retval String& The stored SSID
   */
  const String& getSsid() const {
    return mSsid;
  }

  /**
   * @brief Get the stored WiFi password
   *
   * @retval String& The stored password
   */
  const String& getPassword() const {
    return mPassword;
  }

  /**
   * @brief Set WiFi credentials
   *
   * @param ssid The WiFi network name
   * @param password The WiFi network password
   */
  void setCredentials(const String& ssid, const String& password) {
    mSsid = ssid;
    mPassword = password;
  }

  /**
   * @brief Check if stored credentials are valid
   *
   * Checks if SSID is not empty, which is considered a minimum requirement
   * for valid credentials.
   *
   * @retval true Credentials are valid
   * @retval false Credentials are invalid (SSID is empty)
   */
  bool isCredentialsValid() {
    return !mSsid.isEmpty();
  }

  /**
   * @brief Store WiFi credentials to persistent storage
   *
   * Stores the SSID and password to the CBOR object and persists it to storage.
   *
   * @retval true Storage operation was successful
   * @retval false Storage operation failed (e.g., file system error)
   */
  virtual bool store() override {
    object().put("ssid", mSsid.c_str());
    object().put("pass", mPassword.c_str());
    return CBORStorage::store();
  }

  /**
   * @brief Restore WiFi credentials from persistent storage
   *
   * Loads the CBOR object from storage and extracts SSID and password.
   *
   * @retval true Restore operation was successful
   * @retval false Restore operation failed (e.g., file system error)
   */
  virtual bool restore() override {
    if (CBORStorage::restore()) {
      mSsid = object().getString("ssid");
      mPassword = object().getString("pass");
      return true;
    }
    return false;
  }

  /**
   * @brief Clear stored WiFi credentials
   *
   * Resets SSID and password to empty strings and cleans the storage.
   *
   * @retval true Clean operation was successful
   * @retval false Clean operation failed (e.g., file system error)
   */
  virtual bool clean() override {
    mSsid = "";
    mPassword = "";
    return CBORStorage::clean();
  }

 private:
  String mSsid;     ///< WiFi network name
  String mPassword; ///< WiFi network password
};
/** @} */
}  // namespace uniot
