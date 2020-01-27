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

#include <Credentials.h>

namespace uniot
{
class MQTTPath
{
public:
  MQTTPath(const Credentials &credentials) : mPrefix("PUBLIC_UNIOT"), mpCredentials(&credentials)
  {
  }

  String buildDevicePath(const String &topic)
  {
    return mPrefix + "/"
            + mpCredentials->getOwnerId() + "/"
            + mpCredentials->getDeviceId() + "/"
            + topic;
  }

  String buildGroupPath(const String &topic)
  {
    return mPrefix + "/"
            + mpCredentials->getOwnerId() + "/"
            + topic;
  }

  String buildPublicPath(const String &topic)
  {
    return mPrefix + "/"
            + topic;
  }

  const Credentials *getCredentials() const {
    return mpCredentials;
  }

private:
  const String mPrefix;
  const Credentials *mpCredentials;
};
} // namespace uniot
