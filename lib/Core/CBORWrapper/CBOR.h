/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2020 Uniot <info.uniot@gmail.com>
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

#ifndef MAX_CBOR_BUF_SIZE
#define MAX_CBOR_BUF_SIZE 256
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
    return std::unique_ptr<CBORArray>(new CBORArray(this, mpMapNode, cn_cbor_int_create(key, &mErr)));
  }

  std::unique_ptr<CBORArray> putArray(const char* key) {
    mDirty = true;
    return std::unique_ptr<CBORArray>(new CBORArray(this, mpMapNode, cn_cbor_string_create(key, &mErr)));
  }

  CBOR &put(int key, int value) {
    auto existing = cn_cbor_mapget_int(mpMapNode, key);
    if (existing) {
      mDirty = cn_cbor_int_update(existing, value);
    } else {
      mDirty = cn_cbor_mapput_int(mpMapNode, key, cn_cbor_int_create(value, &mErr), &mErr);
    }
    return *this;
  }

  CBOR &put(int key, const char* value) {
    auto existing = cn_cbor_mapget_int(mpMapNode, key);
    if (existing) {
      mDirty = cn_cbor_string_update(existing, value);
    } else {
      mDirty = cn_cbor_mapput_int(mpMapNode, key, cn_cbor_string_create(value, &mErr), &mErr);
    }
    return *this;
  }

  CBOR &put(const char* key, int value) {
    auto existing = cn_cbor_mapget_string(mpMapNode, key);
    if (existing) {
      mDirty = cn_cbor_int_update(existing, value);
    } else {
      mDirty = cn_cbor_mapput_string(mpMapNode, key, cn_cbor_int_create(value, &mErr), &mErr);
    }
    return *this;
  }

  CBOR &put(const char* key, const char* value) {
    auto existing = cn_cbor_mapget_string(mpMapNode, key);
    if (existing) {
      mDirty = cn_cbor_string_update(existing, value);
    } else {
      mDirty = cn_cbor_mapput_string(mpMapNode, key, cn_cbor_string_create(value, &mErr), &mErr);
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
    mpMapNode = cn_cbor_decode(buf.raw(), buf.size(), &mErr);
    if (!mpMapNode) {
      _create();
    }
  }

  Bytes build() {
    uint8_t buf[MAX_CBOR_BUF_SIZE];
    size_t size = cn_cbor_encoder_write(buf, 0, MAX_CBOR_BUF_SIZE, mpMapNode);
    return Bytes(buf, size);
  }

  bool dirty() {
    return mDirty;
  }

  void clean() {
    _clean();
    _create();
  }

private:
  void _create() {
    mDirty = false;
    mpMapNode = cn_cbor_map_create(&mErr);
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

  cn_cbor* mpMapNode;
  cn_cbor_errback mErr;
  bool mDirty;
};
} // namespace uniot