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

/** @cond */
/**
 * DO NOT DELETE THE "fs_storage" GROUP DEFINITION BELOW.
 * Used to create the File System Storage topic in the documentation. If you want to delete this file,
 * please paste the group definition into another storage and delete this one.
 */
/** @endcond */

/**
 * @defgroup fs_storage File System Storage
 * @brief File system storage management for Uniot devices
 */

#pragma once

// doc: https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html
#if UNIOT_USE_NVSFS == 1
#include <NVSFS.h>
#define FileFS NVSFS
#elif UNIOT_USE_LITTLEFS == 1
#include <LittleFS.h>
#define FileFS LittleFS
#else
#include <FS.h>
#include <SPIFFS.h>
#define FileFS SPIFFS
#endif

#include <Bytes.h>
#include <Logger.h>

namespace uniot {

/**
 * @brief Manages data persistence to file system on ESP8266/ESP32 devices
 * @defgroup fs_storage_base Storage
 * @ingroup fs_storage
 *
 * The Storage class provides a simple interface for saving and restoring data to the
 * file system. It automatically handles filesystem mounting and unmounting based on
 * instance counts. It supports both LittleFS and SPIFFS file systems based on compile-time
 * configuration (UNIOT_USE_LITTLEFS).
 * @{
 */
class Storage {
 public:
  /**
   * @brief Constructs a Storage object for a specific file
   *
   * Creates a Storage instance associated with a specific file path.
   * The constructor automatically mounts the filesystem if this is the first Storage
   * instance created. File path will be prepended with "/" if not already present.
   *
   * @param path The file path to use for storage (limited to 31 characters)
   */
  Storage(const String &path) {
    String normalizedPath = path;
    if (!normalizedPath.startsWith("/")) {
      normalizedPath = "/" + normalizedPath;
    }
    setPath(normalizedPath);

    sInstancesCount++;

    if (!sMounted) {
#if defined(ESP8266)
      sMounted = FileFS.begin();
#elif defined(ESP32)
      sMounted = FileFS.begin(true);
#endif
      // By default, FileFS will automatically format the filesystem if one is not detected.
      // This means that you will lose all filesystem data even if other filesystem exists.
      UNIOT_LOG_WARN_IF(!sMounted, "Failed to mount the file system");
    }
  }

  /**
   * @brief Destructor
   *
   * Decrements the instance counter and unmounts the filesystem
   * when the last instance is destroyed
   */
  virtual ~Storage() {
    sInstancesCount--;
    if (!sInstancesCount) {
      unmount();
    }
  }

  /**
   * @brief Explicitly unmounts the filesystem
   *
   * This method forcibly unmounts the filesystem regardless of
   * how many Storage instances exist. Use with extreme caution as
   * it will affect all other Storage instances.
   *
   * @warning This can cause other Storage instances to fail as they
   * may attempt to access an unmounted filesystem
   */
  static void unmount() {
    if (sMounted) {
      FileFS.end();
      sMounted = false;
    }
  }

  /**
   * @brief Writes the current data to the file system
   *
   * Saves the content of mData to the file specified by mPath.
   * On ESP8266 with SPIFFS, it attempts to run garbage collection
   * to prevent filesystem fragmentation issues.
   *
   * @retval true Data was successfully written to the file
   * @retval false Failed to open the file for writing
   */
  virtual bool store() {
    auto file = FileFS.open(mPath, "w");
    if (!file) {
      UNIOT_LOG_WARN("Failed to open %s", mPath.c_str());
      return false;
    }
    file.write(mData.raw(), mData.size());
    file.close();

#if defined(ESP8266)
#if UNIOT_USE_LITTLEFS != 1
    // using SPIFFS.gc() can avoid or reduce issues where SPIFFS reports
    // free space but is unable to write additional data to a file
    UNIOT_LOG_DEBUG_IF(!FileFS.gc(), "SPIFFS gc failed. That's all right. Caller: %s", mPath.c_str());
#endif
#endif

    return true;
  }

  /**
   * @brief Loads data from the file system into memory
   *
   * Reads the content of the file specified by mPath into mData.
   * If the file doesn't exist (which is normal on first use), it logs
   * a warning but doesn't treat it as an error.
   *
   * @retval true Data was successfully loaded into mData
   * @retval false Failed to open the file for reading
   */
  virtual bool restore() {
    auto file = FileFS.open(mPath, "r");
    if (!file) {
      UNIOT_LOG_WARN("Failed to open %s. It is ok on first start", mPath.c_str());
      return false;
    }
#ifdef UNIOT_USE_NVSFS
    mData = file.getBytes();  // NVSFS provides a convenient way to get bytes directly
#else
    mData = _readSmallFile(file);  // TODO: cleanup before read?
#endif
    file.close();
    return true;
  }

  /**
   * @brief Clears data and removes the file from the file system
   *
   * Cleans up by both clearing the in-memory data and removing
   * the associated file from the filesystem.
   *
   * @retval true File was successfully removed
   * @retval false Failed to remove the file
   */
  virtual bool clean() {
    mData.clean();
    if (!FileFS.remove(mPath)) {
      UNIOT_LOG_WARN("Failed to remove %s", mPath.c_str());
      return false;
    }
    return true;
  }

 protected:
  /**
   * @brief The byte array containing the data to be stored or the loaded data
   *
   * This member stores the actual data that will be written to the file system
   * or that has been read from the file system.
   */
  Bytes mData;

  /**
   * @brief The file path where data is stored
   *
   * This is the full path to the file in the filesystem.
   */
  String mPath;

  /**
   * @brief Sets the file path, ensuring it starts with "/"
   *
   * @param path The file path to set (limited to 31 characters total)
   */
  void setPath(const String &path) {
    mPath = path;
    // There is a limit of 32 chars in total for filenames.
    // One '\0' char is reserved for string termination.
    UNIOT_LOG_WARN_IF(path.length() > 31, "Path length of '%s' > 31 chars", mPath.c_str());
  }

 private:
  /**
   * @brief Helper method to read a file's contents into a Bytes object
   *
   * Reads the entire content of the given file into memory and
   * returns it as a Bytes object.
   *
   * @param file An open File object to read from
   * @retval data The data read from the file
   */

#ifndef UNIOT_USE_NVSFS
  Bytes _readSmallFile(File &file) {
    auto toRead = file.size() - file.position();
    char *buf = (char *)malloc(toRead);
    auto countRead = file.readBytes(buf, toRead);
    const Bytes data((uint8_t *)buf, countRead);
    free(buf);
    return data;
  }
#endif

  /**
   * @brief Tracks whether the filesystem is currently mounted
   *
   * Shared across all Storage instances to prevent multiple mount attempts.
   */
  static bool sMounted;

  /**
   * @brief Tracks the number of active Storage instances
   *
   * Used to determine when to unmount the filesystem (when count reaches zero).
   */
  static unsigned int sInstancesCount;
};
/** @} */
}  // namespace uniot
