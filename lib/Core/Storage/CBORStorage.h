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

// doc: https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html
#include <Storage.h>
#include <CBORObject.h>

namespace uniot
{
/**
 * @brief Storage implementation for CBOR formatted data
 * @defgroup fs_storage_cbor CBOR Storage
 * @ingroup fs_storage
 *
 * CBORStorage extends the base Storage class to provide specific functionality
 * for storing and retrieving data in CBOR (Concise Binary Object Representation) format.
 * This class maintains an internal CBORObject which handles serialization and deserialization
 * of structured data to/from binary format.
 * @{
 */
class CBORStorage : public Storage
{
public:
  /**
   * @brief Constructs a new CBORStorage object
   *
   * @param path The filesystem path where the CBOR data will be stored
   */
  CBORStorage(const String &path) : Storage(path)
  {
  }

  /**
   * @brief Virtual destructor
   */
  virtual ~CBORStorage() {}

  /**
   * @brief Get access to the underlying CBORObject
   *
   * @retval CBORObject& The internal CBOR object
   */
  CBORObject &object()
  {
    return mCbor;
  }

  /**
   * @brief Store the CBOR object to the filesystem
   *
   * Checks if the CBOR object has been modified (is dirty), and if so,
   * serializes it and persists the data to storage.
   *
   * @retval true Storage operation was successful
   * @retval false Storage operation failed (e.g., file system error)
   */
  virtual bool store() override
  {
    if (mCbor.dirty())
    {
      mData = mCbor.build();
      return Storage::store();
    }
    return true;
  }

  /**
   * @brief Restore the CBOR object from the filesystem
   *
   * Loads data from storage and deserializes it into the CBOR object.
   *
   * @retval true Restoration operation was successful
   * @retval false Restoration operation failed (e.g., file system error)
   */
  virtual bool restore() override
  {
    auto success = Storage::restore();
    if (success)
    {
      mCbor.read(mData);
    }
    return success;
  }

  /**
   * @brief Clean the CBOR object and remove the storage file
   *
   * Resets the CBOR object to its initial state and removes the associated file.
   *
   * @retval true Clean operation was successful
   * @retval false Clean operation failed (e.g., file system error)
   */
  virtual bool clean() override
  {
    mCbor.clean();
    return Storage::clean();
  }

protected:
  CBORObject mCbor; ///< The CBOR object used for data serialization/deserialization
};
/** @} */
} // namespace uniot
