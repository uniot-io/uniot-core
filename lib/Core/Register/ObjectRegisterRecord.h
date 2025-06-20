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

#include <ClearQueue.h>
#include <Logger.h>
#include <TypeId.h>

namespace uniot {

/**
 * @brief Base class for objects that can be registered in the ObjectRegister.
 * @defgroup registers_object_register_record Object Register Record
 * @ingroup registers
 * @{
 *
 * This class provides self-registration functionality to track object instances.
 * Any object inheriting from ObjectRegisterRecord is automatically tracked in a
 * static registry upon creation and removed upon destruction. This tracking
 * mechanism helps to detect and handle dangling pointers in the ObjectRegister.
 *
 * Objects of this class cannot be copied, only moved or created/destroyed.
 */
class ObjectRegisterRecord : public IWithType {
 public:
  /**
   * @brief Deleted copy constructor to prevent copying
   */
  ObjectRegisterRecord(const ObjectRegisterRecord &) = delete;

  /**
   * @brief Deleted assignment operator to prevent copying
   */
  void operator=(const ObjectRegisterRecord &) = delete;

  /**
   * @brief Constructor that automatically registers this instance
   *
   * When an instance is created, it is added to a static registry of
   * active ObjectRegisterRecord instances.
   */
  ObjectRegisterRecord() {
    auto success = sRegisteredLinks.pushUnique(this);
    UNIOT_LOG_DEBUG("record.push [%lu][%d]", this, success);
  }

  /**
   * @brief Destructor that automatically unregisters this instance
   *
   * When an instance is destroyed, it is removed from the static
   * registry of active ObjectRegisterRecord instances.
   */
  virtual ~ObjectRegisterRecord() {
    auto success = sRegisteredLinks.removeOne(this);
    UNIOT_LOG_DEBUG("record.remove [%lu][%d]", this, success);
  }

  /**
   * @brief Checks if a record still exists in the registry
   *
   * This static method verifies if a pointer to an ObjectRegisterRecord
   * is still valid by checking if it exists in the registry of active instances.
   *
   * @param record Pointer to the record to check
   * @retval true Record exists in the registry
   * @retval false Record does not exist in the registry
   */
  static bool exists(ObjectRegisterRecord *record) {
    return sRegisteredLinks.contains(record);
  }

 private:
  /**
   * @brief Static collection of all active ObjectRegisterRecord instances
   *
   * This static member maintains a registry of all currently existing
   * ObjectRegisterRecord instances to track their validity.
   */
  static ClearQueue<ObjectRegisterRecord *> sRegisteredLinks;
};
/** @} */
}  // namespace uniot
