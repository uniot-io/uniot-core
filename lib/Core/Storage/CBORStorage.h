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
    return mCbor;
  }

  bool store() override
  {
    if (mCbor.dirty())
    {
      mData = mCbor.build();
      return Storage::store();
    }
    return true;
  }

  bool restore() override
  {
    auto success = Storage::restore();
    if (success)
    {
      mCbor.read(mData);
    }
    return success;
  }

  bool clean() override
  {
    mCbor.clean();
    return Storage::clean();
  }

protected:
  CBOR mCbor;
};
} // namespace uniot