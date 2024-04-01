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

#ifndef UNIOT_DANGEROUS_CBOR_DATA_SIZE
#define UNIOT_DANGEROUS_CBOR_DATA_SIZE 256
#endif

namespace uniot {
class CBORObject {
  friend class COSEMessage;

 public:
  class Array;
  CBORObject(CBORObject const &) = delete;
  void operator=(CBORObject const &) = delete;

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

  CBORObject() : mDirty(false) {
    _create();
  }

  virtual ~CBORObject() {
    _clean();
  }

  cn_cbor_errback getLastError() {
    return mErr;
  }

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

  CBORObject &put(int key, int value) {
    return put(key, static_cast<int64_t>(value));
  }

  CBORObject &put(int key, uint64_t value) {
    return put(key, static_cast<int64_t>(value));
  }

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

  CBORObject &put(int key, const char *value) {
    bool updated = false;
    auto existing = cn_cbor_mapget_int(mpMapNode, key);
    if (existing) {
      updated = cn_cbor_string_update(existing, value);
      if (!updated) {
        UNIOT_LOG_WARN_IF(_isPtrEqual(existing, value), "pointer to the same value is specified for '%s'", key);
      }
    } else {
      updated = cn_cbor_mapput_int(mpMapNode, key, cn_cbor_string_create(value, _errback()), _errback());
    }
    _markAsDirty(updated);
    return *this;
  }

  CBORObject &put(const char *key, int value) {
    return put(key, static_cast<int64_t>(value));
  }

  CBORObject &put(const char *key, uint64_t value) {
    return put(key, static_cast<int64_t>(value));
  }

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

  inline CBORObject getMap(int key) {
    return _getMap(cn_cbor_mapget_int(mpMapNode, key));
  }

  inline CBORObject getMap(const char *key) {
    return _getMap(cn_cbor_mapget_string(mpMapNode, key));
  }

  bool getBool(int key) const {
    return _getBool(cn_cbor_mapget_int(mpMapNode, key));
  }

  bool getBool(const char *key) const {
    return _getBool(cn_cbor_mapget_string(mpMapNode, key));
  }

  long getInt(int key) const {
    return _getInt(cn_cbor_mapget_int(mpMapNode, key));
  }

  long getInt(const char *key) const {
    return _getInt(cn_cbor_mapget_string(mpMapNode, key));
  }

  String getString(int key) const {
    return _getString(cn_cbor_mapget_int(mpMapNode, key));
  }

  String getString(const char *key) const {
    return _getString(cn_cbor_mapget_string(mpMapNode, key));
  }

  String getValueAsString(int key) const {
    return _getValueAsString(cn_cbor_mapget_int(mpMapNode, key));
  }

  String getValueAsString(const char *key) const {
    return _getValueAsString(cn_cbor_mapget_string(mpMapNode, key));
  }

  Bytes getBytes(int key) const {
    return _getBytes(cn_cbor_mapget_int(mpMapNode, key));
  }

  Bytes getBytes(const char *key) const {
    return _getBytes(cn_cbor_mapget_string(mpMapNode, key));
  }

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

  Bytes build() const {
    auto visitSiblings = mpParentObject == nullptr;
    return _build(mpMapNode, visitSiblings);
  }

  bool dirty() const {
    return mDirty;
  }

  void forceDirty() {
    UNIOT_LOG_WARN("the data forced marked as dirty");
    _markAsDirty(true);
  }

  void clean() {
    _clean();
    _create();
  }

  class Array {
    friend class CBORObject;

   public:
    Array(Array const &) = delete;
    void operator=(Array const &) = delete;

    ~Array() {
      mpContext = nullptr;
      mpArrayNode = nullptr;
    }

    cn_cbor_errback getLastError() {
      return mpContext->mErr;
    }

    inline Array &append(int value) {
      if (mpArrayNode) {
        auto updated = cn_cbor_array_append(mpArrayNode, cn_cbor_int_create(value, mpContext->_errback()), mpContext->_errback());
        mpContext->_markAsDirty(updated);
      }
      return *this;
    }

    inline Array &append(const char *value) {
      if (mpArrayNode) {
        auto updated = cn_cbor_array_append(mpArrayNode, cn_cbor_string_create(value, mpContext->_errback()), mpContext->_errback());
        mpContext->_markAsDirty(updated);
      }
      return *this;
    }

    template <typename T>
    inline Array &append(size_t size, const T *value) {
      static_assert(std::is_integral<T>::value, "only integral types are allowed");

      for (size_t i = 0; i < size; i++) {
        append(static_cast<int>(value[i]));
      }
      return *this;
    }

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
    Array(CBORObject *context, cn_cbor *arrayNode)
        : mpContext(context), mpArrayNode(arrayNode) {}

    CBORObject *mpContext;
    cn_cbor *mpArrayNode;
  };

 private:
  CBORObject(CBORObject *parent, cn_cbor *child) : mDirty(false) {
    mErr.err = CN_CBOR_NO_ERROR;
    mErr.pos = 0;
    mpMapNode = child;
    mpParentObject = parent;
  }

  void _create() {
    mErr.err = CN_CBOR_NO_ERROR;
    mErr.pos = 0;
    mDirty = false;
    mpMapNode = cn_cbor_map_create(_errback());
    mpParentObject = nullptr;
  }

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

  Bytes _build(cn_cbor *cb, bool visitSiblings = true) const {
    auto calculated = cn_cbor_encoder_write(NULL, 0, 0, cb, visitSiblings);
    UNIOT_LOG_WARN_IF(calculated > UNIOT_DANGEROUS_CBOR_DATA_SIZE, "dangerous data size: %d", calculated);

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

  inline CBORObject _getMap(cn_cbor *cb) {
    // if(!cb) throw "error"; // TODO: ???
    if (cb && CN_CBOR_MAP == cb->type) {
      return CBORObject(this, cb);
    }

    UNIOT_LOG_WARN("the map is not found");
    return CBORObject();
  }

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

  long _getInt(cn_cbor *cb) const {
    // if(!cb) throw "error"; // TODO: ???
    if (cb && CN_CBOR_INT == cb->type) {
      return cb->v.sint;
    } else if (cb && CN_CBOR_UINT == cb->type) {
      return cb->v.uint;
    }
    return 0;
  }

  String _getString(cn_cbor *cb) const {
    // if(!cb) throw "error"; // TODO: ???
    if (cb && CN_CBOR_TEXT == cb->type) {
      auto bytes = Bytes(cb->v.bytes, cb->length);
      bytes.terminate();
      return String(bytes.c_str());
    }
    return "";
  }

  Bytes _getBytes(cn_cbor *cb) const {
    // if(!cb) throw "error"; // TODO: ???
    if (cb && CN_CBOR_BYTES == cb->type) {
      return Bytes(cb->v.bytes, cb->length);
    }
    return {};
  }

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

  bool _isPtrEqual(cn_cbor *cb, const void *ptr) const {
    return ptr == (const void *)cb->v.bytes;
  }

  void _markAsDirty(bool updated) {
    if (updated) {
      mDirty = true;
      if (mpParentObject) {
        mpParentObject->_markAsDirty(true);
      }
    }
  }

  cn_cbor_errback *_errback() {
    UNIOT_LOG_ERROR_IF(mErr.err, "last unhandled error code: %lu", mErr.err);

    mErr.err = CN_CBOR_NO_ERROR;
    mErr.pos = 0;
    return &mErr;
  }

  CBORObject *mpParentObject;
  cn_cbor *mpMapNode;
  cn_cbor_errback mErr;
  bool mDirty;
  Bytes mBuf;
};
}  // namespace uniot