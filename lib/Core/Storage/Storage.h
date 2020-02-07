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
#include <FS.h>
#include <Bytes.h>

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
      sMounted = SPIFFS.begin();
      if (!sMounted)
      {
        // By default, SPIFFS will autoformat the filesystem if SPIFFS cannot mount it.
        // Use SPIFFSConfig to prevent this behavior.
        Serial.println("Error: failed to mount the file system");
        return;
      }
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
      SPIFFS.end();
      sMounted = false;
    }
  }

  bool store()
  {
    auto file = SPIFFS.open(mPath, "w");
    if (!file)
    {
      Serial.println("Error: Failed to open " + mPath);
      return false;
    }
    file.write(mData.raw(), mData.size());
    file.close();
    // using this call can avoid or reduce issues where SPIFFS reports
    // free space but is unable to write additional data to a file
    if (!SPIFFS.gc())
    {
      Serial.println("Error: SPIFFS gc failed. Caller: " + mPath);
    }
    return true;
  }

  bool restore()
  {
    auto file = SPIFFS.open(mPath, "r");
    if (!file)
    {
      Serial.println("Error: Failed to open " + mPath);
      return false;
    }
    mData = _readSmallFile(file);
    file.close();
    return true;
  }

  bool clean()
  {
    mData.clean();
    if (!SPIFFS.remove(mPath))
    {
      Serial.println("Error: Failed to remove " + mPath);
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
    if (path.length() > 31)
    {
      // There is a limit of 32 chars in total for filenames.
      // One '\0' char is reserved for string termination.
      Serial.println("Warning: path length > 31 chars");
    }
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