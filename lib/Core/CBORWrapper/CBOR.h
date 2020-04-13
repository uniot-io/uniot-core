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

#include <Arduino.h>
#include <Bytes.h>
#include <memory>
#include <cn-cbor.h>

#include "CBORArray.h"

#ifndef UNIOT_DANGEROUS_CBOR_DATA_SIZE
#define UNIOT_DANGEROUS_CBOR_DATA_SIZE 256
#endif

namespace uniot
{
class CBOR
{
  friend class CBORArray;
public:
  CBOR(const Bytes &buf)
      : mpMapNode(nullptr),
        mDirty(false)
  {
    read(buf);
  }

  CBOR() : mDirty(false)
  {
    _create();
  }

  ~CBOR() {
    _clean();
  }

  cn_cbor_errback getLastError() {
    return mErr;
  }

  std::unique_ptr<CBORArray> putArray(int key) {
    mDirty = true;
    return std::unique_ptr<CBORArray>(new CBORArray(this, mpMapNode, cn_cbor_int_create(key, _errback())));
  }

  std::unique_ptr<CBORArray> putArray(const char* key) {
    mDirty = true;
    return std::unique_ptr<CBORArray>(new CBORArray(this, mpMapNode, cn_cbor_string_create(key, _errback())));
  }

  CBOR &put(int key, int value) {
    auto existing = cn_cbor_mapget_int(mpMapNode, key);
    if (existing) {
      mDirty = cn_cbor_int_update(existing, value);
    } else {
      mDirty = cn_cbor_mapput_int(mpMapNode, key, cn_cbor_int_create(value, _errback()), _errback());
    }
    return *this;
  }

  CBOR &put(int key, const char* value) {
    auto existing = cn_cbor_mapget_int(mpMapNode, key);
    if (existing) {
      mDirty = cn_cbor_string_update(existing, value);
      if (!mDirty)
        UNIOT_LOG_WARN_IF(_isPtrEqual(existing, value), "pointer to the same value is specified for '%s'", key);
    } else {
      mDirty = cn_cbor_mapput_int(mpMapNode, key, cn_cbor_string_create(value, _errback()), _errback());
    }
    return *this;
  }

  CBOR &put(const char* key, int value) {
    auto existing = cn_cbor_mapget_string(mpMapNode, key);
    if (existing) {
      mDirty = cn_cbor_int_update(existing, value);
    } else {
      mDirty = cn_cbor_mapput_string(mpMapNode, key, cn_cbor_int_create(value, _errback()), _errback());
    }
    return *this;
  }

  CBOR &put(const char* key, const char* value) {
    auto existing = cn_cbor_mapget_string(mpMapNode, key);
    if (existing) {
      mDirty = cn_cbor_string_update(existing, value);
      if (!mDirty)
        UNIOT_LOG_WARN_IF(_isPtrEqual(existing, value), "pointer to the same value is specified for '%s'", key);
    } else {
      mDirty = cn_cbor_mapput_string(mpMapNode, key, cn_cbor_string_create(value, _errback()), _errback());
    }

    return *this;
  }

  int getInt(int key) const {
    return _getInt(cn_cbor_mapget_int(mpMapNode, key));
  }

  int getInt(const char* key) const {
    return _getInt(cn_cbor_mapget_string(mpMapNode, key));
  }

  String getString(int key) const {
    return _getString(cn_cbor_mapget_int(mpMapNode, key));
  }

  String getString(const char* key) const {
    return _getString(cn_cbor_mapget_string(mpMapNode, key));
  }

  CBOR &copyInt(const CBOR &from, int key) {
    return put(key, from.getInt(key));
  }

  CBOR &copyInt(const CBOR &from, const char* key) {
    return put(key, from.getInt(key));
  }

  CBOR &copyStrPtr(const CBOR &from, int key) {
    auto cb = cn_cbor_mapget_int(from.mpMapNode, key);
    // TODO: check types, set Err
    return put(key, cb->v.str);
  }

  CBOR &copyStrPtr(const CBOR &from, const char* key) {
    auto cb = cn_cbor_mapget_string(from.mpMapNode, key);
    // TODO: check types, set Err
    return put(key, cb->v.str);
  }

  void read(const Bytes &buf) {
    _clean();
    mpMapNode = cn_cbor_decode(buf.raw(), buf.size(), _errback());
    if (!mpMapNode) {
      _create();
    }
  }

  Bytes build()
  {
    // NOTE: for the future, cn_cbor_encoder_calc was not tested with floats
    auto calculated = cn_cbor_encoder_calc(mpMapNode);
    UNIOT_LOG_WARN_IF(calculated > UNIOT_DANGEROUS_CBOR_DATA_SIZE, "dangerous data size: %d", calculated);

    Bytes bytes(nullptr, calculated);
    auto written = bytes.fill([&](uint8_t *buf, size_t size) {
      auto actual = cn_cbor_encoder_write(buf, 0, size, mpMapNode);
      if (actual < 0)
      {
        UNIOT_LOG_ERROR("%s", "CBOR build failed, buffer size too small");
        return 0;
      }
      return actual;
    });

    return bytes.prune(written);
  }

  bool dirty() {
    return mDirty;
  }

  void forceDirty() {
    UNIOT_LOG_WARN("%s", "the data forced marked as dirty");
    mDirty = true;
  }

  void clean() {
    _clean();
    _create();
  }

private:
  void _create() {
    mDirty = false;
    mpMapNode = cn_cbor_map_create(_errback());
  }

  void _clean() {
    if (mpMapNode) {
      cn_cbor_free(mpMapNode);
      mpMapNode = nullptr;
    }
  }

  long _getInt(cn_cbor* cb) const {
    // if(!cb) throw "error"; // TODO: ???
    if(cb && CN_CBOR_INT == cb->type) {
      return cb->v.sint;
    } else if(cb && CN_CBOR_UINT == cb->type) {
      return cb->v.uint;
    }
    return 0;
  }

  String _getString(cn_cbor* cb) const {
    // if(!cb) throw "error"; // TODO: ???
    if(cb && CN_CBOR_TEXT == cb->type) {
      auto bytes = Bytes(cb->v.bytes, cb->length);
      bytes.terminate();
      return String(bytes.c_str());
    }
    return "";
  }

  bool _isPtrEqual(cn_cbor* cb, const void *ptr) const {
    return ptr == (const void *) cb->v.bytes;
  }

  cn_cbor_errback *_errback() {
    UNIOT_LOG_ERROR_IF(mErr.err, "last unhandled error code: %lu", mErr.err);

    mErr.err = CN_CBOR_NO_ERROR;
    mErr.pos = 0;
    return &mErr;
  }

  cn_cbor* mpMapNode;
  cn_cbor_errback mErr;
  bool mDirty;
};
} // namespace uniot