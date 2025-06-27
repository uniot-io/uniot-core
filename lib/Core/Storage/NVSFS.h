/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2025 Uniot <contact@uniot.io>
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
 * @defgroup nvsfs_storage NVSFS Storage
 * @brief Experimental NVS-based file system interface for ESP32 devices
 * @ingroup fs_storage
 *
 * @warning This is an experimental implementation and requires finalization
 * before production use. Some file system interface methods are not yet
 * implemented, and compatibility with the Storage class may be incomplete.
 */

#pragma once

#ifdef ESP32

#include <Bytes.h>
#include <Logger.h>
#include <Preferences.h>

namespace uniot {

/**
 * @brief Experimental file-like interface for NVS storage operations
 * @defgroup nvsfs_file NVSFile
 * @ingroup nvsfs_storage
 *
 * Provides a file system compatible interface for NVS operations on ESP32.
 * This class mimics the behavior of file handles used by SPIFFS and LittleFS,
 * allowing NVS to be used as a drop-in replacement for traditional file systems.
 *
 * @warning This is an experimental implementation. The following limitations exist:
 * - Missing size(), position(), and readBytes() methods required by Storage class
 * - No default constructor for temporary object creation
 * - Limited error handling in some operations
 * - Memory management in _loadData() needs improvement
 *
 * Key differences from traditional file systems:
 * - File paths are converted to NVS keys (max 15 characters)
 * - No concept of file position or streaming reads
 * - Atomic write operations (all-or-nothing)
 * - Built-in wear leveling and data integrity
 * @{
 */
class NVSFile {
 public:
  /**
   * @brief Constructs an NVSFile object for a specific path and mode
   *
   * Creates an NVSFile instance for reading from or writing to NVS storage.
   * The file path is automatically normalized to a valid NVS key by removing
   * leading slashes and truncating to 15 characters.
   *
   * @param path The file path (will be converted to NVS key)
   * @param mode File access mode ("r" for read, "w" for write)
   * @param prefs Reference to the Preferences object for NVS operations
   *
   * @note For read mode, data is immediately loaded from NVS into memory.
   * For write mode, data is buffered and written to NVS only when close() is called.
   */
  NVSFile(const String& path, const String& mode, Preferences& prefs)
      : mKey(path), mMode(mode), mPrefs(prefs), mValid(true) {
    mKey = normalizePath(mKey);
    if (mMode == "r") {
      _loadData();
    } else if (mMode == "w") {
      mBuffer = Bytes();
    } else {
      mValid = false;
    }
  }

  /**
   * @brief Check if the file is valid and ready for operations
   *
   * @retval true File is valid and can be used for read/write operations
   * @retval false File is invalid (wrong mode, NVS error, etc.)
   */
  operator bool() const { return mValid; }

  /**
   * @brief Write data to the file buffer
   *
   * In write mode, stores the provided data in an internal buffer.
   * The data is not actually written to NVS until close() is called.
   * Each call to write() replaces the entire buffer content.
   *
   * @param data Pointer to the data to write
   * @param size Number of bytes to write
   * @retval size Number of bytes written (same as input size on success)
   * @retval 0 Failed to write (invalid file or not in write mode)
   *
   * @warning Current implementation replaces the entire buffer on each write.
   * Append functionality is not yet implemented.
   */
  size_t write(const uint8_t* data, size_t size) {
    if (!mValid || mMode != "w") {
      return 0;
    }

    mBuffer = Bytes(data, size);
    return size;
  }

  /**
   * @brief Get the entire file content as Bytes object
   *
   * Returns a copy of the file's content. In read mode, this is the data
   * loaded from NVS. In write mode, this would be the buffered data.
   *
   * @retval Bytes File content as Bytes object
   * @retval Empty Bytes object if file is invalid or not in read mode
   *
   * @note This method is specific to the NVS implementation and not part
   * of the standard file system interface.
   */
  Bytes getBytes() {
    if (!mValid || mMode != "r") {
      return Bytes();
    }

    return mBuffer;
  }

  /**
   * @brief Close the file and commit any pending write operations
   *
   * For write mode files, this method commits the buffered data to NVS.
   * If the buffer is empty, the corresponding NVS key is removed.
   * For read mode files, this simply marks the file as invalid.
   *
   * After calling close(), the file object becomes invalid and cannot
   * be used for further operations.
   *
   * @note This method should always be called when finished with a file
   * to ensure data is properly saved to NVS.
   */
  void close() {
    if (!mValid) {
      return;
    }

    if (mMode == "w") {
      if (mBuffer.size() == 0) {
        mPrefs.remove(mKey.c_str());
      } else {
        mPrefs.putBytes(mKey.c_str(), mBuffer.raw(), mBuffer.size());
      }
    }

    mValid = false;
  }

  /**
   * @brief Normalize a file path to a valid NVS key
   *
   * Converts a file system path to a valid NVS key by:
   * - Removing leading forward slash "/"
   * - Truncating to 15 characters maximum (NVS limitation)
   * - Logging a warning if truncation occurs
   *
   * @param path The original file path
   * @return Normalized NVS key string
   *
   * @note This is a static utility method that can be used independently
   * of any NVSFile instance.
   */
  static String normalizePath(const String& path) {
    String normalized = path;
    if (normalized.startsWith("/")) {
      normalized = normalized.substring(1);
    }
    if (normalized.length() > 15) {
      normalized = normalized.substring(0, 15);
      UNIOT_LOG_WARN("NVS key truncated to 15 chars: %s", normalized.c_str());
    }
    return normalized;
  }

 private:
  String mKey;          ///< Normalized NVS key derived from file path
  String mMode;         ///< File access mode ("r" or "w")
  Bytes mBuffer;        ///< Internal data buffer
  Preferences& mPrefs;  ///< Reference to NVS Preferences object
  bool mValid;          ///< Whether the file is valid for operations

  /**
   * @brief Load data from NVS into the internal buffer
   *
   * Called automatically when opening a file in read mode.
   * Retrieves the data associated with the NVS key and stores
   * it in the internal buffer for subsequent read operations.
   *
   * @warning Current implementation uses Bytes::fill() which may
   * need review for memory safety and error handling.
   */
  void _loadData() {
    if (!mPrefs.isKey(mKey.c_str())) {
      mBuffer = Bytes();
      return;
    }

    size_t dataSize = mPrefs.getBytesLength(mKey.c_str());
    if (dataSize == 0) {
      mBuffer = Bytes();
      return;
    }

    mBuffer = Bytes(nullptr, dataSize);
    mBuffer.fill([this, dataSize](uint8_t* buf, size_t size) {
      return mPrefs.getBytes(mKey.c_str(), buf, dataSize);
    });
  }
};
/** @} */

/**
 * @brief Experimental NVS-based file system interface for ESP32
 * @defgroup nvsfs_filesystem NVSFileSystem
 * @ingroup nvsfs_storage
 *
 * Provides a file system compatible interface using ESP32's NVS (Non-Volatile Storage)
 * as the backend. This allows NVS to be used as a drop-in replacement for SPIFFS
 * or LittleFS in applications that require faster access times and better wear leveling.
 *
 * @warning This is an experimental implementation with the following limitations:
 * - Missing exists() method for file existence checking
 * - Missing gc() method for garbage collection compatibility
 * - Error handling in open() method needs improvement
 * - No support for directory operations
 *
 * Advantages over traditional file systems:
 * - Faster read/write operations (no file system overhead)
 * - Built-in wear leveling and atomic operations
 * - Better suited for configuration data and small files
 * - No file system mounting/unmounting required
 *
 * Limitations:
 * - Maximum 15-character file names (NVS key limitation)
 * - No hierarchical directory structure
 * - Less suitable for large files or binary data
 * @{
 */
class NVSFileSystem {
 public:
  /**
   * @brief Default constructor
   *
   * Creates an uninitialized NVS file system instance.
   * Call begin() before performing any file operations.
   */
  NVSFileSystem() : mInitialized(false) {}

  /**
   * @brief Initialize the NVS file system
   *
   * Opens the NVS namespace used for file storage operations.
   * This must be called before any file operations can be performed.
   *
   * @param formatOnFail Currently unused (NVS doesn't require formatting)
   * @retval true NVS namespace opened successfully
   * @retval false Failed to open NVS namespace
   *
   * @note The method uses a fixed namespace "uniot_files" for all file operations.
   * This ensures isolation from other NVS usage in the application.
   */
  bool begin(bool formatOnFail = false) {
    if (mInitialized) return true;

    mInitialized = mPrefs.begin("uniot_files", false);
    if (!mInitialized) {
      UNIOT_LOG_ERROR("Failed to open NVS namespace 'uniot_files'");
    }

    return mInitialized;
  }

  /**
   * @brief Deinitialize the NVS file system
   *
   * Closes the NVS namespace and marks the file system as uninitialized.
   * Any subsequent file operations will fail until begin() is called again.
   */
  void end() {
    if (mInitialized) {
      mPrefs.end();
      mInitialized = false;
    }
  }

  /**
   * @brief Open a file for reading or writing
   *
   * Creates an NVSFile object for the specified path and access mode.
   * The path is automatically normalized to a valid NVS key.
   *
   * @param path File path (will be converted to NVS key)
   * @param mode Access mode ("r" for read, "w" for write)
   * @return NVSFile object for the specified file
   *
   * @warning If the file system is not initialized, returns an invalid
   * NVSFile object. Always check the returned object with operator bool().
   *
   * @note Currently only supports "r" and "w" modes. Append mode ("a")
   * and other modes are not yet implemented.
   */
  NVSFile open(const String& path, const String& mode) {
    if (!mInitialized) {
      UNIOT_LOG_WARN("NVS not initialized");
      return NVSFile("", "", mPrefs);
    }
    return NVSFile(path, mode, mPrefs);
  }

  /**
   * @brief Remove a file from NVS storage
   *
   * Deletes the NVS key corresponding to the specified file path.
   * The operation is immediate and cannot be undone.
   *
   * @param path File path to remove
   * @retval true File was successfully removed
   * @retval false Failed to remove file (may not exist or NVS error)
   *
   * @note If the file doesn't exist, the operation may still return true
   * depending on the underlying NVS implementation.
   */
  bool remove(const String& path) {
    if (!mInitialized) {
      return false;
    }

    String key = NVSFile::normalizePath(path);
    return mPrefs.remove(key.c_str());
  }

 private:
  Preferences mPrefs;  ///< NVS Preferences object for storage operations
  bool mInitialized;   ///< Whether the file system has been initialized
};
/** @} */

/**
 * @brief Global NVS file system instance
 *
 * Provides a global NVSFS object that can be used as a drop-in replacement
 * for SPIFFS or LittleFS in the Storage class and other file system operations.
 *
 * Usage example:
 * @code
 * #define FileFS NVSFS  // Use NVS instead of SPIFFS/LittleFS
 * @endcode
 */
extern NVSFileSystem NVSFS;

}  // namespace uniot

#else
#error "NVSFS is only supported on ESP32. Please define UNIOT_USE_NVSFS=1 for ESP32 projects."
#endif