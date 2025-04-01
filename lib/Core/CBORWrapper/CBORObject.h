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

#include <Arduino.h>
#include <Bytes.h>
#include <Logger.h>
#include <cn-cbor.h>

#include <memory>

namespace uniot {
/**
 * @brief A wrapper class for CBOR (Concise Binary Object Representation) data manipulation
 * @defgroup utils_cbor_wrapper CBOR (Concise Binary Object Representation)
 * @ingroup utils
 * @{
 *
 * CBORObject provides an interface to create, read, modify, and build CBOR data.
 * It wraps the cn-cbor library to handle CBOR encoding and decoding operations.
 * This class supports map operations with string or integer keys, and provides
 * methods to work with various data types (int, string, bytes, arrays, and nested maps).
 */
class CBORObject {
  friend class COSEMessage;

 public:
  class Array;

  /**
   * @brief Copy constructor (not implemented)
   * @param other The CBORObject to copy
   * @note This constructor logs a warning since copying is not implemented
   */
  CBORObject(const CBORObject &) : mDirty(false) {
    _create();
    UNIOT_LOG_WARN("Copy constructor is not implemented!");
  }

  /**
   * @brief Copy assignment operator (not implemented)
   * @param other The CBORObject to assign from
   * @retval CBORObject& Reference to this object
   * @note This operator logs a warning since copying is not implemented
   */
  CBORObject &operator=(const CBORObject &) {
    UNIOT_LOG_WARN("Copy assignment operator is not implemented!");
    return *this;
  }

  /**
   * @brief Construct a CBORObject from binary CBOR data
   * @param buf The CBOR data in Bytes format
   */
  CBORObject(Bytes buf)
      : mpParentObject(nullptr),
        mpMapNode(nullptr),
        mDirty(false) {
    mErr.err = CN_CBOR_NO_ERROR;
    mErr.pos = 0;

    // NOTE: In C++, when a constructor of a derived class is executing, the object is not yet of the derived class type;
    // it's still of the base class type.
    // This means that virtual functions do not behave polymorphically within constructors (and destructors).
    // If `read` is overridden in a class derived from `CBORObject`,
    // that overridden version will not be called in the `CBORObject` constructor.
    read(buf);
  }

  /**
   * @brief Construct an empty CBORObject
   * @note Creates an empty CBOR map
   */
  CBORObject() : mDirty(false) {
    _create();
  }

  /**
   * @brief Virtual destructor
   * @note Cleans up resources if this is a root object
   */
  virtual ~CBORObject() {
    _clean();
  }

  /**
   * @brief Get the last error that occurred during CBOR operations
   * @retval err The error code
   */
  cn_cbor_errback getLastError() {
    return mErr;
  }

  /**
   * @brief Create or get an array at a specific integer key
   * @param key The integer key
   * @retval Array Array object representing the array
   * @note If the key already exists and is an array, returns that array.
   *       Otherwise, creates a new array at the key.
   */
  Array putArray(int key) {
    auto existing = cn_cbor_mapget_int(mpMapNode, key);
    if (existing && existing->type == CN_CBOR_ARRAY) {
      return Array(this, existing);
    } else {
      auto newArray = cn_cbor_array_create(_errback());
      if (newArray) {
        if (cn_cbor_mapput_int(mpMapNode, key, newArray, _errback())) {
          _markAsDirty(true);
          return Array(this, newArray);
        } else {
          cn_cbor_free(newArray);
        }
      }
    }
    return Array(this, nullptr);
  }

  /**
   * @brief Create or get an array at a specific string key
   * @param key The string key
   * @retval Array Array object representing the array
   * @note If the key already exists and is an array, returns that array.
   *       Otherwise, creates a new array at the key.
   */
  inline Array putArray(const char *key) {
    auto existing = cn_cbor_mapget_string(mpMapNode, key);
    if (existing && existing->type == CN_CBOR_ARRAY) {
      return Array(this, existing);
    } else {
      auto newArray = cn_cbor_array_create(_errback());
      if (newArray) {
        if (cn_cbor_mapput_string(mpMapNode, key, newArray, _errback())) {
          _markAsDirty(true);
          return Array(this, newArray);
        } else {
          cn_cbor_free(newArray);
        }
      }
    }
    return Array(this, nullptr);
  }

  /**
   * @brief Put an integer value at a specific integer key
   * @param key The integer key
   * @param value The integer value to store
   * @retval CBORObject& Reference to this object
   */
  CBORObject &put(int key, int value) {
    return put(key, static_cast<int64_t>(value));
  }

  /**
   * @brief Put an unsigned 64-bit integer value at a specific integer key
   * @param key The integer key
   * @param value The unsigned 64-bit integer value to store
   * @retval CBORObject& Reference to this object
   */
  CBORObject &put(int key, uint64_t value) {
    return put(key, static_cast<int64_t>(value));
  }

  /**
   * @brief Put a 64-bit integer value at a specific integer key
   * @param key The integer key
   * @param value The 64-bit integer value to store
   * @retval CBORObject& Reference to this object
   * @note Updates the value if the key already exists
   */
  CBORObject &put(int key, int64_t value) {
    bool updated = false;
    auto existing = cn_cbor_mapget_int(mpMapNode, key);
    if (existing) {
      updated = cn_cbor_int_update(existing, value);
    } else {
      updated = cn_cbor_mapput_int(mpMapNode, key, cn_cbor_int_create(value, _errback()), _errback());
    }
    _markAsDirty(updated);
    return *this;
  }

  /**
   * @brief Put a string value at a specific integer key
   * @param key The integer key
   * @param value The string value to store
   * @retval CBORObject& Reference to this object
   * @note Updates the value if the key already exists
   */
  CBORObject &put(int key, const char *value) {
    bool updated = false;
    auto existing = cn_cbor_mapget_int(mpMapNode, key);
    if (existing) {
      updated = cn_cbor_string_update(existing, value);
      if (!updated) {
        UNIOT_LOG_WARN_IF(_isPtrEqual(existing, value), "pointer to the same value is specified for '%d'", key);
      }
    } else {
      updated = cn_cbor_mapput_int(mpMapNode, key, cn_cbor_string_create(value, _errback()), _errback());
    }
    _markAsDirty(updated);
    return *this;
  }

  /**
   * @brief Put binary data at a specific integer key
   * @param key The integer key
   * @param value Pointer to the binary data
   * @param size Size of the binary data in bytes
   * @retval CBORObject& Reference to this object
   * @note Updates the value if the key already exists
   */
  CBORObject &put(int key, const uint8_t *value, int size) {
    bool updated = false;
    auto existing = cn_cbor_mapget_int(mpMapNode, key);
    if (existing) {
      updated = cn_cbor_data_update(existing, value, size);
      if (!updated) {
        UNIOT_LOG_WARN_IF(_isPtrEqual(existing, value), "pointer to the same value is specified for '%d'", key);
      }
    } else {
      updated = cn_cbor_mapput_int(mpMapNode, key, cn_cbor_data_create(value, size, _errback()), _errback());
    }
    _markAsDirty(updated);
    return *this;
  }

  /**
   * @brief Put an integer value at a specific string key
   * @param key The string key
   * @param value The integer value to store
   * @retval CBORObject& Reference to this object
   */
  CBORObject &put(const char *key, int value) {
    return put(key, static_cast<int64_t>(value));
  }

  /**
   * @brief Put an unsigned 64-bit integer value at a specific string key
   * @param key The string key
   * @param value The unsigned 64-bit integer value to store
   * @retval CBORObject& Reference to this object
   */
  CBORObject &put(const char *key, uint64_t value) {
    return put(key, static_cast<int64_t>(value));
  }

  /**
   * @brief Put a 64-bit integer value at a specific string key
   * @param key The string key
   * @param value The 64-bit integer value to store
   * @retval CBORObject& Reference to this object
   * @note Updates the value if the key already exists
   */
  CBORObject &put(const char *key, int64_t value) {
    bool updated = false;
    auto existing = cn_cbor_mapget_string(mpMapNode, key);
    if (existing) {
      updated = cn_cbor_int_update(existing, value);
    } else {
      updated = cn_cbor_mapput_string(mpMapNode, key, cn_cbor_int_create(value, _errback()), _errback());
    }
    _markAsDirty(updated);
    return *this;
  }

  /**
   * @brief Put a string value at a specific string key
   * @param key The string key
   * @param value The string value to store
   * @retval CBORObject& Reference to this object
   * @note Updates the value if the key already exists
   */
  CBORObject &put(const char *key, const char *value) {
    bool updated = false;
    auto existing = cn_cbor_mapget_string(mpMapNode, key);
    if (existing) {
      updated = cn_cbor_string_update(existing, value);
      if (!updated) {
        UNIOT_LOG_WARN_IF(_isPtrEqual(existing, value), "pointer to the same value is specified for '%s'", key);
      }
    } else {
      updated = cn_cbor_mapput_string(mpMapNode, key, cn_cbor_string_create(value, _errback()), _errback());
    }
    _markAsDirty(updated);
    return *this;
  }

  /**
   * @brief Put binary data at a specific string key
   * @param key The string key
   * @param value Pointer to the binary data
   * @param size Size of the binary data in bytes
   * @retval CBORObject& Reference to this object
   * @note Updates the value if the key already exists
   */
  CBORObject &put(const char *key, const uint8_t *value, int size) {
    bool updated = false;
    auto existing = cn_cbor_mapget_string(mpMapNode, key);
    if (existing) {
      updated = cn_cbor_data_update(existing, value, size);
      if (!updated) {
        UNIOT_LOG_WARN_IF(_isPtrEqual(existing, value), "pointer to the same value is specified for '%s'", key);
      }
    } else {
      updated = cn_cbor_mapput_string(mpMapNode, key, cn_cbor_data_create(value, size, _errback()), _errback());
    }
    _markAsDirty(updated);
    return *this;
  }

  /**
   * @brief Put a new map at a specific string key, or get the existing one
   * @param key The string key
   * @retval CBORObject A new CBORObject representing the map
   * @note If the key already has a map, returns that map
   */
  inline CBORObject putMap(const char *key) {
    auto existing = cn_cbor_mapget_string(mpMapNode, key);
    if (existing) {
      return _getMap(existing);
    }

    auto newMap = cn_cbor_map_create(_errback());
    auto success = cn_cbor_mapput_string(mpMapNode, key, newMap, _errback());
    if (success) {
      _markAsDirty(true);
      return CBORObject(this, newMap);
    }
    return {};
  }

  /**
   * @brief Get a map at a specific integer key
   * @param key The integer key
   * @retval CBORObject A new CBORObject representing the map
   * @retval CBORObject An empty object if the key does not exist or is not a map
   */
  inline CBORObject getMap(int key) {
    return _getMap(cn_cbor_mapget_int(mpMapNode, key));
  }

  /**
   * @brief Get a map at a specific string key
   * @param key The string key
   * @retval CBORObject A new CBORObject representing the map
   * @retval CBORObject An empty object if the key does not exist or is not a map
   */
  inline CBORObject getMap(const char *key) {
    return _getMap(cn_cbor_mapget_string(mpMapNode, key));
  }

  /**
   * @brief Get a boolean value at a specific integer key
   * @param key The integer key
   * @retval bool The boolean value
   * @retval false If the key does not exist or is not a boolean
   */
  bool getBool(int key) const {
    return _getBool(cn_cbor_mapget_int(mpMapNode, key));
  }

  /**
   * @brief Get a boolean value at a specific string key
   * @param key The string key
   * @retval bool The boolean value
   * @retval false If the key does not exist or is not a boolean
   */
  bool getBool(const char *key) const {
    return _getBool(cn_cbor_mapget_string(mpMapNode, key));
  }

  /**
   * @brief Get an integer value at a specific integer key
   * @param key The integer key
   * @retval long The integer value
   * @retval 0 If the key does not exist or is not an integer
   */
  long getInt(int key) const {
    return _getInt(cn_cbor_mapget_int(mpMapNode, key));
  }

  /**
   * @brief Get an integer value at a specific string key
   * @param key The string key
   * @retval long The integer value
   * @retval 0 If the key does not exist or is not an integer
   */
  long getInt(const char *key) const {
    return _getInt(cn_cbor_mapget_string(mpMapNode, key));
  }

  /**
   * @brief Get a string value at a specific integer key
   * @param key The integer key
   * @retval String The string value
   * @retval "" Empty string, if the key does not exist or is not a string
   */
  String getString(int key) const {
    return _getString(cn_cbor_mapget_int(mpMapNode, key));
  }

  /**
   * @brief Get a string value at a specific string key
   * @param key The string key
   * @retval String The string value
   * @retval "" Empty string, if the key does not exist or is not a string
   */
  String getString(const char *key) const {
    return _getString(cn_cbor_mapget_string(mpMapNode, key));
  }

  /**
   * @brief Get a value as a string at a specific integer key
   * @param key The integer key
   * @retval String The value converted to a string
   * @retval "" Empty string, if the key does not exist or is not a string
   * @note Converts various types (numbers, booleans, strings) to string representation
   */
  String getValueAsString(int key) const {
    return _getValueAsString(cn_cbor_mapget_int(mpMapNode, key));
  }

  /**
   * @brief Get a value as a string at a specific string key
   * @param key The string key
   * @retval String The value converted to a string
   * @retval "" Empty string, if the key does not exist or is not a string
   * @note Converts various types (numbers, booleans, strings) to string representation
   */
  String getValueAsString(const char *key) const {
    return _getValueAsString(cn_cbor_mapget_string(mpMapNode, key));
  }

  /**
   * @brief Get binary data at a specific integer key
   * @param key The integer key
   * @retval Bytes The binary data
   * @retval Bytes Empty if the key does not exist or is not binary data
   */
  Bytes getBytes(int key) const {
    return _getBytes(cn_cbor_mapget_int(mpMapNode, key));
  }

  /**
   * @brief Get binary data at a specific string key
   * @param key The string key
   * @retval Bytes The binary data
   * @retval Bytes Empty if the key does not exist or is not binary data
   */
  Bytes getBytes(const char *key) const {
    return _getBytes(cn_cbor_mapget_string(mpMapNode, key));
  }

  /**
   * @brief Read CBOR data from a buffer
   * @param buf The buffer containing CBOR data
   * @note This operation fails if the object is a child node
   */
  void read(const Bytes &buf) {
    if (mpParentObject) {
      UNIOT_LOG_WARN("the parent node is not null, the object is not read");
      return;
    }

    _clean();
    mBuf = buf;
    mpMapNode = cn_cbor_decode(mBuf.raw(), mBuf.size(), _errback());
    if (!mpMapNode) {
      _create();
    }
  }

  /**
   * @brief Build the CBOR data into binary format
   * @param visitSiblings Whether to include sibling nodes (default is true for root objects)
   * @retval Bytes The binary CBOR data
   */
  Bytes build() const {
    auto visitSiblings = mpParentObject == nullptr;
    return _build(mpMapNode, visitSiblings);
  }

  /**
   * @brief Check if this object is a child node in a CBOR tree
   * @retval true Current object is a child of another CBORObject
   * @retval false Current object is a root CBORObject
   */
  bool isChild() const {
    return mpParentObject != nullptr;
  }

  /**
   * @brief Check if the object has been modified since creation or last read
   * @retval true Object is dirty (modified)
   * @retval false Object is clean (not modified)
   */
  bool dirty() const {
    return mDirty;
  }

  /**
   * @brief Force the object to be marked as dirty (modified)
   * @note This is useful when modifications were made outside the class's interface
   */
  void forceDirty() {
    UNIOT_LOG_WARN("the data forced marked as dirty");
    _markAsDirty(true);
  }

  /**
   * @brief Reset the object to an empty state
   * @note Cleans up resources and creates a new empty CBOR map
   */
  void clean() {
    _clean();
    _create();
  }

  /**
   * @brief Helper class for working with CBOR arrays
   *
   * This class provides methods to append values to a CBOR array
   * and maintains a reference back to the parent CBORObject.
   */
  class Array {
    friend class CBORObject;

   public:
    /**
     * @brief Copy constructor
     * @param other The Array to copy
     */
    Array(const Array &other)
        : mpContext(other.mpContext),
          mpArrayNode(other.mpArrayNode) {
    }

    /**
     * @brief Copy assignment operator
     * @param other The Array to copy from
     * @retval Array& Reference to this Array
     */
    Array &operator=(const Array &other) {
      if (this != &other) {
        mpContext = other.mpContext;
        mpArrayNode = other.mpArrayNode;
      }
      return *this;
    }

    /**
     * @brief Destructor
     */
    ~Array() {
      mpContext = nullptr;
      mpArrayNode = nullptr;
    }

    /**
     * @brief Get the last error that occurred during array operations
     * @retval error The error information
     */
    cn_cbor_errback getLastError() {
      return mpContext->mErr;
    }

    /**
     * @brief Append an integer to the array
     * @param value The integer value to append
     * @retval Array& Reference to this Array
     */
    inline Array &append(int value) {
      if (mpArrayNode) {
        auto updated = cn_cbor_array_append(mpArrayNode, cn_cbor_int_create(value, mpContext->_errback()), mpContext->_errback());
        mpContext->_markAsDirty(updated);
      }
      return *this;
    }

    /**
     * @brief Append a string to the array
     * @param value The string value to append
     * @retval Array& Reference to this Array
     */
    inline Array &append(const char *value) {
      if (mpArrayNode) {
        auto updated = cn_cbor_array_append(mpArrayNode, cn_cbor_string_create(value, mpContext->_errback()), mpContext->_errback());
        mpContext->_markAsDirty(updated);
      }
      return *this;
    }

    /**
     * @brief Append multiple values from an array to the CBOR array
     * @tparam T The array element type (must be integral)
     * @param size The number of elements to append
     * @param value Pointer to the array of values
     * @retval Array& Reference to this Array
     */
    template <typename T>
    inline Array &append(size_t size, const T *value) {
      static_assert(std::is_integral<T>::value, "only integral types are allowed");

      for (size_t i = 0; i < size; i++) {
        append(static_cast<int>(value[i]));
      }
      return *this;
    }

    /**
     * @brief Append a new array as an element
     * @retval Array A new Array object representing the nested array
     */
    inline Array appendArray() {
      auto newArray = cn_cbor_array_create(mpContext->_errback());
      if (newArray) {
        auto updated = cn_cbor_array_append(mpArrayNode, newArray, mpContext->_errback());
        mpContext->_markAsDirty(updated);
        if (updated) {
          return Array(mpContext, newArray);
        } else {
          cn_cbor_free(newArray);
        }
      }
      return Array(mpContext, nullptr);
    }

   private:
    /**
     * @brief Private constructor used by CBORObject
     * @param context The parent CBORObject
     * @param arrayNode The underlying cn_cbor array node
     */
    Array(CBORObject *context, cn_cbor *arrayNode)
        : mpContext(context), mpArrayNode(arrayNode) {}

    CBORObject *mpContext;  ///< Parent CBORObject
    cn_cbor *mpArrayNode;   ///< Underlying cn_cbor array node
  };

 private:
  /**
   * @brief Private constructor for child objects
   * @param parent The parent CBORObject
   * @param child The underlying cn_cbor node
   */
  CBORObject(CBORObject *parent, cn_cbor *child) : mDirty(false) {
    mErr.err = CN_CBOR_NO_ERROR;
    mErr.pos = 0;
    mpMapNode = child;
    mpParentObject = parent;
  }

  /**
   * @brief Create an empty CBOR map
   * @note Initializes internal state and creates a new map node
   */
  void _create() {
    mErr.err = CN_CBOR_NO_ERROR;
    mErr.pos = 0;
    mDirty = false;
    mpMapNode = cn_cbor_map_create(_errback());
    mpParentObject = nullptr;
  }

  /**
   * @brief Clean up resources
   * @note For root objects, frees the map node and resets internal state
   */
  void _clean() {
    if (!mpParentObject) {
      if (mpMapNode) {
        cn_cbor_free(mpMapNode);
      }
    }
    mpMapNode = nullptr;
    mpParentObject = nullptr;
    mDirty = false;
    mErr.err = CN_CBOR_NO_ERROR;
    mErr.pos = 0;
    mBuf.clean();
  }

  /**
   * @brief Build CBOR data from a node
   * @param cb The cn_cbor node to build from
   * @param visitSiblings Whether to include sibling nodes
   * @retval Bytes The binary CBOR data
   */
  Bytes _build(cn_cbor *cb, bool visitSiblings = true) const {
    auto calculated = cn_cbor_encoder_write(NULL, 0, 0, cb, visitSiblings);
    Bytes bytes(nullptr, calculated);
    auto written = bytes.fill([&](uint8_t *buf, size_t size) {
      auto actual = cn_cbor_encoder_write(buf, 0, size, cb, visitSiblings);
      if (actual < 0) {
        UNIOT_LOG_ERROR("%s", "CBORObject build failed, buffer size too small");
        return 0;
      }
      return actual;
    });

    return bytes.prune(written);
  }

  /**
   * @brief Helper to get a map from a cn_cbor node
   * @param cb The cn_cbor node
   * @retval CBORObject A new CBORObject representing the map
   * @retval CBORObject An empty object if the node is not a map
   */
  inline CBORObject _getMap(cn_cbor *cb) {
    // if(!cb) throw "error"; // TODO: ???
    if (cb && CN_CBOR_MAP == cb->type) {
      return CBORObject(this, cb);
    }

    UNIOT_LOG_WARN("the map is not found");
    return CBORObject();
  }

  /**
   * @brief Helper to get a boolean value from a cn_cbor node
   * @param cb The cn_cbor node
   * @retval bool The boolean value
   * @retval false If the key does not exist or is not a boolean
   */
  long _getBool(cn_cbor *cb) const {
    // if(!cb) throw "error"; // TODO: ???
    if (cb) {
      if (CN_CBOR_TRUE == cb->type) {
        return true;
      }
      if (CN_CBOR_FALSE == cb->type) {
        return false;
      }
    }
    return false;
  }

  /**
   * @brief Helper to get an integer value from a cn_cbor node
   * @param cb The cn_cbor node
   * @retval long The integer value
   * @retval 0 If the key does not exist or is not an integer
   */
  long _getInt(cn_cbor *cb) const {
    // if(!cb) throw "error"; // TODO: ???
    if (cb && CN_CBOR_INT == cb->type) {
      return cb->v.sint;
    } else if (cb && CN_CBOR_UINT == cb->type) {
      return cb->v.uint;
    }
    return 0;
  }

  /**
   * @brief Helper to get a string value from a cn_cbor node
   * @param cb The cn_cbor node
   * @retval String The string value
   * @retval "" Empty string, if the key does not exist or is not a string
   */
  String _getString(cn_cbor *cb) const {
    // if(!cb) throw "error"; // TODO: ???
    if (cb && CN_CBOR_TEXT == cb->type) {
      auto bytes = Bytes(cb->v.bytes, cb->length);
      bytes.terminate();
      return String(bytes.c_str());
    }
    return "";
  }

  /**
   * @brief Helper to get binary data from a cn_cbor node
   * @param cb The cn_cbor node
   * @retval Bytes The binary data
   * @retval Bytes Empty if the key does not exist or is not binary data
   */
  Bytes _getBytes(cn_cbor *cb) const {
    // if(!cb) throw "error"; // TODO: ???
    if (cb && CN_CBOR_BYTES == cb->type) {
      return Bytes(cb->v.bytes, cb->length);
    }
    return {};
  }

  /**
   * @brief Helper to convert various types to string representation
   * @param cb The cn_cbor node
   * @retval String The value converted to a string
   * @retval "" Empty string, if the key does not exist or is not a string
   */
  String _getValueAsString(cn_cbor *cb) const {
    // if(!cb) throw "error"; // TODO: ???
    if (cb) {
      if (CN_CBOR_TEXT == cb->type) {
        auto bytes = Bytes(cb->v.bytes, cb->length);
        bytes.terminate();
        return String(bytes.c_str());
      }
      if (CN_CBOR_INT == cb->type) {
        return String(cb->v.sint);
      }
      if (CN_CBOR_UINT == cb->type) {
        return String(cb->v.uint);
      }
      if (CN_CBOR_FLOAT == cb->type) {
        return String(cb->v.f);
      }
      if (CN_CBOR_DOUBLE == cb->type) {
        return String(cb->v.dbl);
      }
      if (CN_CBOR_TRUE == cb->type) {
        return String("1");
      }
      if (CN_CBOR_FALSE == cb->type) {
        return String("0");
      }
    }
    return "";
  }

  /**
   * @brief Check if a cn_cbor node's data pointer equals a given pointer
   * @param cb The cn_cbor node
   * @param ptr The pointer to compare with
   * @retval true The pointers are equal
   * @retval false The pointers are not equal
   */
  bool _isPtrEqual(cn_cbor *cb, const void *ptr) const {
    return ptr == (const void *)cb->v.bytes;
  }

  /**
   * @brief Mark the object as dirty (modified)
   * @param updated Whether an update occurred
   * @note Propagates the dirty state to parent objects
   */
  void _markAsDirty(bool updated) {
    if (updated) {
      mDirty = true;
      if (mpParentObject) {
        mpParentObject->_markAsDirty(true);
      }
    }
  }

  /**
   * @brief Get a pointer to the error structure for cn_cbor operations
   * @retval cn_cbor_errback* Pointer to the error structure
   * @note Logs an error if there was a previous unhandled error
   */
  cn_cbor_errback *_errback() {
    UNIOT_LOG_ERROR_IF(mErr.err, "last unhandled error code: %lu", mErr.err);

    mErr.err = CN_CBOR_NO_ERROR;
    mErr.pos = 0;
    return &mErr;
  }

  Bytes mBuf;                   ///< Storage for the original CBOR data
  CBORObject *mpParentObject;   ///< Parent object (null for root objects)
  cn_cbor *mpMapNode;           ///< The underlying cn_cbor node
  cn_cbor_errback mErr;         ///< Error information structure
  bool mDirty;                  ///< Whether the object has been modified
};
/** @} */
}  // namespace uniot
