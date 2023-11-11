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
#include <CBORStorage.h>
#include <Crypto.h>
#include <Ed25519.h>
#include <RNG.h>
#include <user_interface.h>

namespace uniot {
class Credentials : public CBORStorage {
 public:
  Credentials() : CBORStorage("credentials.cbor") {
    mCreatorId = UNIOT_CREATOR_ID;
    mDeviceId = _calcDeviceId();
    Credentials::restore();

    if (mPrivateKey.size() == 0) {
      _generatePrivateKey();
      Credentials::store();
    }
    _derivePublicKey();
  }

  bool store() override {
    object().put("account", mOwnerId.c_str());
    object().put("private_key", mPrivateKey.raw(), mPrivateKey.size());
    return CBORStorage::store();
  }

  bool restore() override {
    if (CBORStorage::restore()) {
      mOwnerId = object().getString("account");
      mPrivateKey = object().getBytes("private_key");
      return true;
    }
    UNIOT_LOG_ERROR("%s", "credentials not restored");
    return false;
  }

  void setOwnerId(const String &id) {
    mOwnerId = id;
  }

  const String &getOwnerId() const {
    return mOwnerId;
  }

  const String &getCreatorId() const {
    return mCreatorId;
  }

  const String &getDeviceId() const {
    return mDeviceId;
  }

  const String &getPublicKey() const {
    return mPublicKey;
  }

  uint32_t getShortDeviceId() const {
    return ESP.getChipId();
  }

  Bytes sign(const Bytes &data) const {
    uint8_t signature[64];
    uint8_t publicKey[32];
    Ed25519::derivePublicKey(publicKey, mPrivateKey.raw());
    Ed25519::sign(signature, mPrivateKey.raw(), publicKey, data.raw(), data.size());
    return Bytes(signature, sizeof(signature));
  }

 private:
  String _calcDeviceId() {
    uint8_t mac[6];
    char macStr[13] = {0};
    wifi_get_macaddr(STATION_IF, mac);

    for (uint8_t i = 0; i < 6; i++)
      sprintf(macStr + i * 2, "%02x", mac[i]);

    return String(macStr);
  }

  void _generatePrivateKey() {
    RNG.begin(String("uniot::entropy::" + mCreatorId + "::" + mDeviceId).c_str());

    uint8_t privateKey[32];
    Ed25519::generatePrivateKey(privateKey);
    mPrivateKey = Bytes(privateKey, sizeof(privateKey));
  }

  void _derivePublicKey() {
    uint8_t publicKey[32];
    Ed25519::derivePublicKey(publicKey, mPrivateKey.raw());
    mPublicKey = Bytes(publicKey, sizeof(publicKey)).toHexString();
  }

  String mOwnerId;
  String mCreatorId;
  String mDeviceId;
  Bytes mPrivateKey;
  String mPublicKey;
};
}  // namespace uniot
