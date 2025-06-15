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

/**
 * @file Credentials.h
 * @defgroup device-identity Device Identity
 * @brief Handles device identity and cryptographic credentials for Uniot devices.
 *
 * This module provides the Credentials class which manages device identifiers,
 * cryptographic keys, and signing operations for Uniot devices. It supports
 * secure storage and retrieval of device credentials using CBOR serialization.
 */

#pragma once

#include <Arduino.h>
#include <CBORStorage.h>
#include <Crypto.h>
#include <Ed25519.h>
#include <ICOSESigner.h>
#include <RNG.h>

#if defined(ESP8266)
  #include <user_interface.h>
#elif defined(ESP32)
  #include <esp_wifi.h>
  #include <esp_system.h>
#endif

namespace uniot {
/**
 * @brief Manages device identity and cryptographic credentials for Uniot devices.
 * @ingroup device-identity
 * @{
 *
 * The Credentials class is responsible for:
 * - Generating and storing device private keys
 * - Deriving public keys from private keys
 * - Signing data using the device's private key
 * - Managing device and owner identifiers
 * - Providing persistent storage of credentials
 *
 * It inherits from CBORStorage for serialization/deserialization and
 * from ICOSESigner to implement COSE signing capabilities.
 */
class Credentials : public CBORStorage, public ICOSESigner {
 public:
  /**
   * @brief Constructor that initializes device credentials.
   *
   * On first instantiation, it generates a new Ed25519 key pair for the device.
   * On subsequent instantiations, it loads existing credentials from persistent storage.
   */
  Credentials() : CBORStorage("credentials.cbor"), mOnwerChanged(false) {
    mCreatorId = UNIOT_CREATOR_ID;
    mDeviceId = _calcDeviceId();
    Credentials::restore();

    if (mPrivateKey.size() == 0) {
      _generatePrivateKey();
      Credentials::store();
    }
    _derivePublicKey();
  }

  /**
   * @brief Stores credentials to persistent storage.
   *
   * Saves the owner ID and private key to the CBOR storage.
   *
   * @retval true Storage operation was successful.
   * @retval false Storage operation failed.
   */
  virtual bool store() override {
    object().put("account", mOwnerId.c_str());
    object().put("private_key", mPrivateKey.raw(), mPrivateKey.size());
    return CBORStorage::store();
  }

  /**
   * @brief Restores credentials from persistent storage.
   *
   * Loads the owner ID and private key from CBOR storage.
   *
   * @retval true Credentials were successfully restored.
   * @retval false Credentials could not be restored.
   */
  virtual bool restore() override {
    if (CBORStorage::restore()) {
      mOwnerId = object().getString("account");
      mPrivateKey = object().getBytes("private_key");
      return true;
    }
    UNIOT_LOG_ERROR("%s", "credentials not restored");
    return false;
  }

  /**
   * @brief Sets the owner ID of the device.
   *
   * @param id The new owner ID to set.
   */
  void setOwnerId(const String &id) {
    if (mOwnerId != id) {
      mOnwerChanged = true;
    }
    mOwnerId = id;
  }

  /**
   * @brief Gets the current owner ID.
   *
   * @retval ownerId& The owner ID.
   */
  const String &getOwnerId() const {
    return mOwnerId;
  }

  bool isOwnerChanged() const {
    return mOnwerChanged;
  }

  void resetOwnerChanged() {
    mOnwerChanged = false;
  }

  /**
   * @brief Gets the creator ID.
   *
   * @retval creatorId& The creator ID.
   */
  const String &getCreatorId() const {
    return mCreatorId;
  }

  /**
   * @brief Gets the unique device ID.
   *
   * @retval deviceId& The device ID.
   */
  const String &getDeviceId() const {
    return mDeviceId;
  }

  /**
   * @brief Gets the device's public key as a hexadecimal string.
   *
   * @retval publicKey& The public key in hexadecimal format.
   */
  const String &getPublicKey() const {
    return mPublicKey;
  }

  /**
   * @brief Gets a shorter unique identifier for the device.
   *
   * Uses ESP-specific functions to obtain a chip ID.
   *
   * @retval uint32_t The short device ID.
   */
  uint32_t getShortDeviceId() const {
#if defined(ESP8266)
    return ESP.getChipId();
#elif defined(ESP32)
    uint64_t mac = ESP.getEfuseMac();
    return (uint32_t)(mac >> 32); // Use the higher 32 bits of the MAC as the Chip ID
#endif
  }

  /**
   * @brief Implements ICOSESigner interface to provide key ID.
   *
   * @retval Bytes The raw public key bytes.
   */
  virtual Bytes keyId() const override {
    return mPublicKeyRaw;
  }

  /**
   * @brief Implements ICOSESigner interface to sign data.
   *
   * Signs the provided data using the device's Ed25519 private key.
   *
   * @param data The data to sign.
   * @retval Bytes The signature of the data.
   */
  virtual Bytes sign(const Bytes &data) const override {
    uint8_t signature[64];
    uint8_t publicKey[32];
    Ed25519::derivePublicKey(publicKey, mPrivateKey.raw());
    Ed25519::sign(signature, mPrivateKey.raw(), publicKey, data.raw(), data.size());
    return Bytes(signature, sizeof(signature));
  }

  /**
   * @brief Implements ICOSESigner interface to specify the signing algorithm.
   *
   * @retval COSEAlgorithm::EdDSA The algorithm used for signing.
   */
  virtual COSEAlgorithm signerAlgorithm() const override {
    return COSEAlgorithm::EdDSA;
  }

 private:
  /**
   * @brief Calculates the device ID based on MAC address.
   *
   * Obtains the WiFi MAC address and formats it as a hexadecimal string.
   *
   * @retval deviceId A String containing the hexadecimal representation of the MAC address.
   */
  String _calcDeviceId() {
    uint8_t mac[6];
    char macStr[13] = {0};
#if defined(ESP8266)
    wifi_get_macaddr(STATION_IF, mac);
#elif defined(ESP32)
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
#endif
    for (uint8_t i = 0; i < 6; i++)
      sprintf(macStr + i * 2, "%02x", mac[i]);

    return String(macStr);
  }

  /**
   * @brief Generates a new Ed25519 private key.
   *
   * Uses device-specific entropy to generate a secure private key.
   */
  void _generatePrivateKey() {
    // Initialize the random number generator with device-specific entropy
    RNG.begin(String("uniot::entropy::" + mCreatorId + "::" + mDeviceId).c_str());

    uint8_t privateKey[32];
    Ed25519::generatePrivateKey(privateKey);
    mPrivateKey = Bytes(privateKey, sizeof(privateKey));
  }

  /**
   * @brief Derives the public key from the private key.
   *
   * Computes both raw binary and hexadecimal string representations.
   */
  void _derivePublicKey() {
    uint8_t publicKey[32];
    Ed25519::derivePublicKey(publicKey, mPrivateKey.raw());
    mPublicKeyRaw = Bytes(publicKey, sizeof(publicKey));
    mPublicKey = mPublicKeyRaw.toHexString();
  }

  String mOwnerId;      ///< Account identifier of the device owner
  String mCreatorId;    ///< Identifier of the creator/manufacturer
  String mDeviceId;     ///< Device unique identifier based on MAC address
  Bytes mPrivateKey;    ///< Ed25519 private key (32 bytes)
  Bytes mPublicKeyRaw;  ///< Raw binary representation of the Ed25519 public key
  String mPublicKey;    ///< Hexadecimal string representation of the public key

  bool mOnwerChanged;
};
/** @} */
}  // namespace uniot
