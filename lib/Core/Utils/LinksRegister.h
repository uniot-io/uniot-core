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
#include <Common.h>
#include <Map.h>
#include <LinkRegisterRecord.h>

namespace uniot
{

class LinksRegister
{
public:
  using RecordPtr = LinkRegisterRecord *;

  LinksRegister() {}
  virtual ~LinksRegister() {}

  LinksRegister(LinksRegister const &) = delete;
  void operator=(LinksRegister const &) = delete;

  bool link(const String &name, RecordPtr link)
  {
    if (!mMapLinks.exist(name))
      mMapLinks.put(name, SharedPointer<IterableQueue<RecordPtr>>(new IterableQueue<RecordPtr>()));

    return mMapLinks.get(name)->pushUnique(link);
  }

  template <typename T>
  T *first(const String &name)
  {
    if (!mMapLinks.exist(name))
      return nullptr;
    auto links = mMapLinks.get(name);

    links->begin();
    return _get<T>(links);
  }

  template <typename T>
  T *next(const String &name)
  {
    if (!mMapLinks.exist(name))
      return nullptr;
    auto links = mMapLinks.get(name);

    if (links->isEnd())
      return nullptr;

    links->next();
    return _get<T>(links);
  }

private:
  template <typename T>
  T *_get(SharedPointer<IterableQueue<RecordPtr>> links)
  {
    if (!links->isEnd())
    {
      auto link = links->current();
      // UNIOT_LOG_DEBUG("record.access [%lu]", link);
      if (LinkRegisterRecord::exists(link))
      {
        return static_cast<T *>(link);
      }
      else
      {
        UNIOT_LOG_DEBUG("record not exists [%lu]", link);
        links->removeOne(link);
      }
    }
    return nullptr;
  }

  Map<String, SharedPointer<IterableQueue<RecordPtr>>> mMapLinks;
};

} // namespace uniot
