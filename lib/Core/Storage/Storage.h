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

// doc: https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html
#include <LittleFS.h>
#include <Bytes.h>
#include <Logger.h>

namespace uniot
{
class Storage
{
public:
  Storage(const String &path)
  {
    setPath(path);

    sInstancesCount++;

    if (!sMounted)
    {
      sMounted = LittleFS.begin();
      // By default, LittleFS will automatically format the filesystem if one is not detected.
      // This means that you will lose all filesystem data even if other filesystem exists.
      UNIOT_LOG_WARN_IF(!sMounted, "Failed to mount the file system");
    }
  }

  virtual ~Storage()
  {
    sInstancesCount--;
    if (!sInstancesCount)
    {
      unmount();
    }
  }

  // use with caution, but it is better not to use it at all!
  static void unmount()
  {
    if (sMounted)
    {
      LittleFS.end();
      sMounted = false;
    }
  }

  virtual bool store()
  {
    auto file = LittleFS.open(mPath, "w");
    if (!file)
    {
      UNIOT_LOG_WARN("Failed to open %s", mPath.c_str());
      return false;
    }
    file.write(mData.raw(), mData.size());
    file.close();
    return true;
  }

  virtual bool restore()
  {
    auto file = LittleFS.open(mPath, "r");
    if (!file)
    {
      UNIOT_LOG_WARN("Failed to open %s. It is ok on first start", mPath.c_str());
      return false;
    }
    mData = _readSmallFile(file);
    file.close();
    return true;
  }

  virtual bool clean()
  {
    mData.clean();
    if (!LittleFS.remove(mPath))
    {
      UNIOT_LOG_WARN("Failed to remove %s", mPath.c_str());
      return false;
    }
    return true;
  }

protected:
  Bytes mData;
  String mPath;

  void setPath(const String &path)
  {
    mPath = path;
    // There is a limit of 32 chars in total for filenames.
    // One '\0' char is reserved for string termination.
    UNIOT_LOG_WARN_IF(path.length() > 31, "Path length of '%s' > 31 chars", mPath.c_str());
  }

private:
  Bytes _readSmallFile(File &file)
  {
    auto toRead = file.size() - file.position();
    char *buf = (char *)malloc(toRead);
    auto countRead = file.readBytes(buf, toRead);
    const Bytes data((uint8_t *)buf, countRead);
    free(buf);
    return data;
  }

  static bool sMounted;
  static unsigned int sInstancesCount;
};
} // namespace uniot