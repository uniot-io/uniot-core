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

// doc: https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html
#include <Storage.h>
#include <CBOR.h>

namespace uniot
{
class CBORStorage : public Storage
{
public:
  CBORStorage(const String &path) : Storage(path)
  {
  }

  virtual ~CBORStorage() {}

  CBOR &object()
  {
    return mCborFrom;
  }

  bool store()
  {
    migrate(mCborFrom, mCborTo);

    auto *cbor = mCborTo.dirty() ? &mCborTo : &mCborFrom;
    if (cbor->dirty()) // nothing to store if mCborFrom does not dirty
    {
      mData = cbor->build();
      return Storage::store();
    }
    return true;
  }

  bool restore()
  {
    auto success = Storage::restore();
    if (success)
    {
      mCborFrom.read(mData);
    }
    return success;
  }

  // D o not be afraid to use to.copyStrPtr(from, key), it is safe.
  // All strings are stored in mData buffer.
  // The mData buffer will be replaced by newly created Bytes when storing data.
  virtual void migrate(const CBOR &from, CBOR &to) = 0;

protected:
  CBOR mCborFrom;
  CBOR mCborTo;
};
} // namespace uniot