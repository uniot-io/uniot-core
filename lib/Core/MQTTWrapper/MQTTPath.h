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

#include <Credentials.h>

namespace uniot
{
/**
 * @brief Utility class for generating standardized MQTT topic paths
 * @defgroup mqtt_path MQTT Path
 * @ingroup mqtt_communication
 *
 * MQTTPath provides a set of methods to construct properly formatted
 * MQTT topic paths for various scopes (device, group, public) based on
 * device credentials. It ensures consistent topic structure throughout
 * the Uniot ecosystem.
 * @{
 */
class MQTTPath
{
public:
  /**
   * @brief Constructs an MQTTPath instance with device credentials
   *
   * @param credentials Device credentials containing owner and device identifiers
   */
  MQTTPath(const Credentials &credentials) : mPrefix("PUBLIC_UNIOT"), mpCredentials(&credentials)
  {
  }

  /**
   * @brief Gets the device ID from the credentials
   *
   * @retval String& Reference to the device ID string
   */
  inline const String& getDeviceId() const
  {
    return mpCredentials->getDeviceId();
  }

  /**
   * @brief Gets the owner ID from the credentials
   *
   * @retval String& Reference to the owner ID string
   */
  inline const String& getOwnerId() const
  {
    return mpCredentials->getOwnerId();
  }

  /**
   * @brief Builds an MQTT topic path for device-specific communication
   *
   * Creates a topic path with the structure:
   * PUBLIC_UNIOT/users/{ownerId}/devices/{deviceId}/{topic}
   *
   * @param topic The specific subtopic for the device
   * @retval path The complete MQTT topic path
   */
  String buildDevicePath(const String &topic) const
  {
    return mPrefix + "/"
            + "users/"
            + mpCredentials->getOwnerId() + "/"
            + "devices/"
            + mpCredentials->getDeviceId() + "/"
            + topic;
  }

  /**
   * @brief Builds an MQTT topic path for group-level communication
   *
   * Creates a topic path with the structure:
   * PUBLIC_UNIOT/users/{ownerId}/groups/{groupId}/{topic}
   *
   * @param groupId The identifier for the device group
   * @param topic The specific subtopic for the group
   * @retval path The complete MQTT topic path
   */
  String buildGroupPath(const String& groupId, const String &topic) const
  {
    return mPrefix + "/"
            + "users/"
            + mpCredentials->getOwnerId() + "/"
            + "groups/"
            + groupId + "/"
            + topic;
  }

  /**
   * @brief Builds an MQTT topic path for public communication
   *
   * Creates a topic path with the structure:
   * PUBLIC_UNIOT/{topic}
   *
   * @param topic The specific public topic
   * @retval path The complete MQTT topic path
   */
  String buildPublicPath(const String &topic) const
  {
    return mPrefix + "/"
            + topic;
  }

  /**
   * @brief Gets the credentials object used by this instance
   *
   * @retval Credentials* Pointer to the credentials object
   */
  const Credentials *getCredentials() const
  {
    return mpCredentials;
  }

private:
  /**
   * @brief The prefix used for all MQTT paths ("PUBLIC_UNIOT")
   */
  const String mPrefix;

  /**
   * @brief Pointer to device credentials (not owned by this class)
   */
  const Credentials *mpCredentials;
};
/** @} */
} // namespace uniot
