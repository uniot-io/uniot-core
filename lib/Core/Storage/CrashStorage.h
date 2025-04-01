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
 * @file CrashStorage.h
 * @defgroup fs_storage_crash Crash Storage
 * @ingroup fs_storage
 * @brief Handles crash data storage and retrieval for ESP8266
 *
 * This file defines the CrashStorage class, which is responsible for
 * capturing and storing crash information from the ESP8266 system.
 * It includes methods for saving crash data to persistent storage,
 * cleaning up crash data, and printing crash information to Serial.
 */

#pragma once


#if defined (ESP8266)
#include <user_interface.h>
#include <Storage.h>

#include <Arduino.h>

namespace uniot
{
/**
 * @brief ESP8266 crash callback function registered with system
 * @ingroup fs_storage_crash
 *
 * Called by the ESP8266 system when a crash occurs to save crash information
 * to persistent storage for later analysis.
 *
 * @param resetInfo Crash reason and register information from ESP8266
 * @param stackStart Start address of the stack to be dumped
 * @param stackEnd End address of the stack to be dumped
 */
void uniotCrashCallback(struct rst_info *resetInfo, uint32_t stackStart, uint32_t stackEnd);

/**
 * @brief Handles crash data storage and retrieval for ESP8266
 * @ingroup fs_storage_crash
 *
 * CrashStorage extends Storage to provide crash dump functionality.
 * It captures crash information including exception details, register values,
 * and a stack trace when a crash occurs, saving this information to a file
 * for later diagnostics.
 * @{
 */
class CrashStorage : public Storage
{
public:
  /**
   * @brief Constructs a CrashStorage object
   *
   * @param path The file path where crash information will be stored
   */
  CrashStorage(const String &path)
    : Storage(path)
  {
  }

  virtual ~CrashStorage()
  {
  }

  /**
   * @brief Stores crash information to a file
   *
   * Builds the crash dump data and writes it to storage.
   *
   * @retval true Storage operation was successful
   * @retval false Storage operation failed (e.g., file system error)
   */
  bool store() override;

  /**
   * @brief Clears crash information and removes file
   *
   * Resets crash data members and cleans the stored file.
   *
   * @retval true Clean operation was successful
   * @retval false Clean operation failed (e.g., file system error)
   */
  bool clean() override;

  /**
   * @brief Outputs crash data to Serial if it exists
   *
   * Checks if crash data is available and prints it to the Serial output.
   *
   * @retval true Crash data was printed to Serial
   * @retval false No crash data exists to print
   */
  bool printCrashDataIfExists() const;

protected:
  /**
   * @brief Generates formatted crash information including stack dump
   *
   * Creates a formatted text report of crash information including:
   * - Crash time and reason
   * - Exception information
   * - Register values
   * - Stack dump
   *
   * @retval Bytes Formatted crash report data
   */
  Bytes buildDumpData() const;

  struct rst_info *mResetInfo;  ///< ESP8266 reset/crash information
  uint32_t mStackStart;         ///< Start address of stack to be dumped
  uint32_t mStackEnd;           ///< End address of stack to be dumped

private:
  friend void uniotCrashCallback(struct rst_info *resetInfo, uint32_t stackStart, uint32_t stackEnd);

  /**
   * @brief Sets crash information received from the crash callback
   *
   * @param resetInfo ESP8266 reset/crash information
   * @param stackStart Start address of stack to be dumped
   * @param stackEnd End address of stack to be dumped
   */
  void setCrashInfo(struct rst_info* resetInfo, uint32_t stackStart, uint32_t stackEnd)
  {
    mResetInfo = resetInfo;
    mStackStart = stackStart;
    mStackEnd = stackEnd;
  }
};
/** @} */
} // namespace uniot

#endif // defined(ESP8266)
