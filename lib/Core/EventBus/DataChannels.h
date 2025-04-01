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
#include <LimitedQueue.h>
#include <Map.h>

namespace uniot {

/**
 * @brief A templated class that manages multiple data channels with queue-like behavior.
 * @defgroup data_channels Data Channels
 * @ingroup event_bus
 *
 * DataChannels provides a way to create, manage and communicate through multiple
 * independent data channels, each identified by a unique channel identifier.
 * Each channel operates as a limited-size FIFO queue with automatic overwriting
 * of the oldest data when full.
 *
 * @tparam T_channel The type used for channel identification (typically an integer or enum)
 * @tparam T_data The data type to be stored and transmitted through the channels
 *
 * @{
 */
template <class T_channel, class T_data>
class DataChannels {
 public:
  virtual ~DataChannels() = default;

  /**
   * @brief Creates a new data channel with the specified capacity
   *
   * @param channel The unique identifier for the channel
   * @param limit The maximum number of data items the channel can hold
   * @retval true If the channel was successfully created
   * @retval false If a channel with the same identifier already exists
   */
  bool open(T_channel channel, size_t limit);

  /**
   * @brief Removes an existing channel and frees associated resources
   *
   * @param channel The identifier of the channel to close
   * @retval true If the channel was successfully closed
   * @retval false If the channel doesn't exist
   */
  bool close(T_channel channel);

  /**
   * @brief Sends data to the specified channel
   *
   * If the channel is full, the oldest data will be overwritten.
   *
   * @param channel The identifier of the destination channel
   * @param data The data to send
   * @retval true If the data was successfully sent
   * @retval false If the channel doesn't exist
   */
  bool send(T_channel channel, T_data data);

  /**
   * @brief Retrieves and removes the oldest data from the specified channel
   *
   * @param channel The identifier of the source channel
   * @retval T_data The retrieved data, or an empty/default data object if the channel
   *                doesn't exist or is empty
   */
  T_data receive(T_channel channel);

  /**
   * @brief Checks if a channel has no data available
   *
   * @param channel The identifier of the channel to check
   * @retval true If the channel is empty or doesn't exist
   * @retval false If the channel exists and contains data
   */
  bool isEmpty(T_channel channel);

 private:
  /** Map of channels, each associated with a limited queue for data storage */
  Map<T_channel, SharedPointer<LimitedQueue<T_data>>> mChannels;
};
/** @} */

}  // namespace uniot
