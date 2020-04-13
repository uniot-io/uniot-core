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

#include <Storage.h>

#include <Arduino.h>

namespace uniot
{
void uniotCrashCallback(struct rst_info *resetInfo, uint32_t stackStart, uint32_t stackEnd);

class CrashStorage : public Storage
{
public:
  CrashStorage(const String &path)
    : Storage(path)
  {
  }

  virtual ~CrashStorage()
  {
  }

  bool store() override;
  bool clean() override;
  bool printCrashDataIfExists() const;

protected:
  Bytes buildDumpData() const;

  struct rst_info *mResetInfo;
  uint32_t mStackStart;
  uint32_t mStackEnd;

private:
  friend void uniotCrashCallback(struct rst_info *resetInfo, uint32_t stackStart, uint32_t stackEnd);
  void setCrashInfo(struct rst_info* resetInfo, uint32_t stackStart, uint32_t stackEnd)
  {
    mResetInfo = resetInfo;
    mStackStart = stackStart;
    mStackEnd = stackEnd;
  }
};
} // namespace uniot
