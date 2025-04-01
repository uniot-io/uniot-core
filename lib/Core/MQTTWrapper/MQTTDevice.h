/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2023 Uniot <contact@uniot.io>
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

#include <Bytes.h>
#include <IterableQueue.h>

namespace uniot {
class MQTTKit;

/**
 * @brief Base class for MQTT-enabled devices in the Uniot ecosystem.
 * @defgroup mqtt_device MQTT Device
 * @ingroup mqtt_communication
 *
 * MQTTDevice provides a common interface for all MQTT-connected devices.
 * It manages topic subscriptions and provides methods for publishing messages
 * to device-specific, group, or arbitrary topics.
 * @{
 */
class MQTTDevice {
  friend class MQTTKit;

 public:
  /**
   * @brief Constructs a new MQTTDevice instance.
   */
  MQTTDevice() : mpKit(nullptr) {}

  /**
   * @brief Virtual destructor that handles cleanup and unregistration from MQTTKit.
   */
  virtual ~MQTTDevice();

  /**
   * @brief Gets the device identifier.
   * @retval deviceId& A reference to the device ID string
   * @retval sEmptyString Empty string if not connected to a kit.
   */
  const String &getDeviceId() const;

  /**
   * @brief Gets the owner identifier.
   * @retval ownerId& A reference to the owner ID string
   * @retval sEmptyString Empty string if not connected to a kit.
   */
  const String &getOwnerId() const;

  /**
   * @brief Subscribes to a specific MQTT topic.
   * @param topic The complete MQTT topic string to subscribe to.
   * @retval String& Reference to the stored topic string.
   */
  const String &subscribe(const String &topic);

  /**
   * @brief Subscribes to a device-specific subtopic.
   * @param subTopic The subtopic to append to the device base path.
   * @retval String& A reference to the stored topic string.
   * @retval sEmptyString Empty string if not connected to a kit.
   */
  const String &subscribeDevice(const String &subTopic);

  /**
   * @brief Subscribes to a group-specific subtopic.
   * @param groupId The identifier of the group to subscribe to.
   * @param subTopic The subtopic to append to the group base path.
   * @retval String& A reference to the stored topic string.
   * @retval sEmptyString Empty string if not connected to a kit.
   */
  const String &subscribeGroup(const String &groupId, const String &subTopic);

  /**
   * @brief Unsubscribes from a specific topic.
   * @param topic The topic to unsubscribe from.
   * @retval true Successfully unsubscribed.
   * @retval false Failed to unsubscribe (e.g., not subscribed).
   */
  bool unsubscribe(const String &topic);

  /**
   * @brief Reconstructs subscriptions after reconnection or credential changes.
   *
   * Implementing classes should override this to set up all required subscriptions.
   *
   * @note Subscriptions that depend on credentials should be reconstructed here
   */
  virtual void syncSubscriptions() = 0;

  /**
   * @brief Unsubscribes from all subscribed topics.
   */
  void unsubscribeFromAll();

  /**
   * @brief Checks if the device is subscribed to a given topic.
   * @param topic The topic to check for subscription.
   * @retval true The device is subscribed to the topic.
   * @retval false The device is not subscribed to the topic.
   */
  bool isSubscribed(const String &topic);

  /**
   * @brief Determines if a stored topic matches an incoming topic string using MQTT wildcards.
   *
   * Implements MQTT topic matching with support for + (single-level) and # (multi-level) wildcards.
   *
   * @param storedTopic The topic string with possible wildcards (+ or #) the device is subscribed to.
   * @param incomingTopic The concrete topic string from an incoming message.
   * @retval true The topics match.
   * @retval false The topics do not match.
   */
  bool isTopicMatch(const String &storedTopic, const String &incomingTopic) const;

  /**
   * @brief Publishes a message to a specific topic.
   * @param topic The complete topic to publish to.
   * @param payload The message payload.
   * @param retained Whether the message should be retained by the broker.
   * @param sign Whether to cryptographically sign the message.
   */
  void publish(const String &topic, const Bytes &payload, bool retained = false, bool sign = false);

  /**
   * @brief Publishes a message to a device-specific subtopic.
   * @param subTopic The subtopic to append to the device base path.
   * @param payload The message payload.
   * @param retained Whether the message should be retained by the broker.
   * @param sign Whether to cryptographically sign the message.
   */
  void publishDevice(const String &subTopic, const Bytes &payload, bool retained = false, bool sign = false);

  /**
   * @brief Publishes a message to a group-specific subtopic.
   * @param groupId The identifier of the group to publish to.
   * @param subTopic The subtopic to append to the group base path.
   * @param payload The message payload.
   * @param retained Whether the message should be retained by the broker.
   * @param sign Whether to cryptographically sign the message.
   */
  void publishGroup(const String &groupId, const String &subTopic, const Bytes &payload, bool retained = false, bool sign = false);

  /**
   * @brief Publishes an empty message to a device-specific subtopic with retained flag set.
   * @param subTopic The subtopic to append to the device base path.
   *
   * This is commonly used to clear retained messages from the broker.
   */
  void publishEmptyDevice(const String &subTopic);

 protected:
  /**
   * @brief Handles incoming MQTT messages.
   * @param topic The topic the message was received on.
   * @param payload The message payload.
   *
   * Implementing classes must override this to process incoming messages.
   */
  virtual void handle(const String &topic, const Bytes &payload) = 0;

 private:
  /**
   * @brief Provides access to the topics queue.
   * @retval mTopics Pointer to the queue of subscribed topics.
   */
  IterableQueue<String> *topics() {
    return &mTopics;
  }

  /**
   * @brief Sets the associated MQTTKit instance.
   * @param kit Pointer to the MQTTKit instance.
   */
  void kit(MQTTKit *kit) {
    mpKit = kit;
  }

  /** Queue of topics the device is subscribed to */
  IterableQueue<String> mTopics;

  /** Pointer to the associated MQTTKit instance */
  MQTTKit *mpKit;

  /** Static empty string for safe returns when needed */
  static const String sEmptyString;
};
/** @} */
}  // namespace uniot
