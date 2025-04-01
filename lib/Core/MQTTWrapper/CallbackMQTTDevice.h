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

#include <functional>
#include <MQTTDevice.h>
#include <Logger.h>

namespace uniot
{

/**
 * @brief MQTT device implementation that delegates message handling to a callback function.
 * @defgroup callback_mqtt_device Callback Device
 * @ingroup mqtt_communication
 *
 * CallbackMQTTDevice extends the base MQTTDevice class by allowing the user to provide
 * a custom handler function that will be called whenever an MQTT message is received.
 * This enables flexible message processing without subclassing.
 * @{
 */
class CallbackMQTTDevice : public MQTTDevice
{
public:
  /**
   * @brief Function type for MQTT message handlers.
   *
   * @param device Pointer to the MQTT device that received the message
   * @param topic The MQTT topic the message was received on
   * @param payload The message payload as bytes
   */
  using Handler = std::function<void(MQTTDevice *device, const String &topic, const Bytes &payload)>;

  /**
   * @brief Constructs a CallbackMQTTDevice with the specified message handler.
   *
   * @param handler The callback function that will process incoming MQTT messages
   */
  CallbackMQTTDevice(Handler handler)
      : MQTTDevice(),
        mHandler(handler)
  {
  }

protected:
  /**
   * @brief Handles incoming MQTT messages by delegating to the callback function.
   *
   * This method overrides the base class implementation to log the topic and
   * invoke the user-provided handler function.
   *
   * @param topic The MQTT topic the message was received on
   * @param payload The message payload as bytes
   */
  void handle(const String &topic, const Bytes &payload) override
  {
    UNIOT_LOG_DEBUG("topic: %s", topic.c_str());
    mHandler(this, topic, payload);
  }

  /**
   * @brief The user-provided message handler function.
   */
  Handler mHandler;
};
/** @} */
} // namespace uniot
