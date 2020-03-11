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
#include <Logger.h>

namespace uniot
{
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

    bool store()
    {
      mData = buildDumpData();
      return Storage::store();
    }

    bool clean()
    {
      mResetInfo = nullptr;
      mStackStart = 0;
      mStackEnd = 0;

      return Storage::clean();
    }

    bool printCrashDataIfExists() const
    {
      if (mData.size())
      {
        UNIOT_LOG_WARN("Crash file dump:");
        Serial.print(mData.c_str());
        return true;
      }
      return false;
    }

  protected:
    void setCrashInfo(struct rst_info* resetInfo, uint32_t stackStart, uint32_t stackEnd)
    {
      mResetInfo = resetInfo;
      mStackStart = stackStart;
      mStackEnd = stackEnd;
    }

    Bytes buildDumpData() const
    {
      const uint32_t crashTime = millis();

      // // one complete log has 170 chars + 45 * n
      // // n >= 2 e [2, 4, 6, ...]
      // // 4220 + safety will last for 90 stack traces including header (170)
      // // uint16_t maximumFileContent = 4250;

      // maximum tmpBuffer size needed is 83, so 100 should be enough
      const int16_t stackLength = mStackEnd - mStackStart;
      char *tmpBuffer = (char*)calloc(170 + 45 * stackLength, sizeof(char));

      // max. 65 chars of Crash time, reason, exception
      int numBytesWritten = sprintf(tmpBuffer, "Crashed at %d ms\nRestart reason: %d\nException (%d):\n", crashTime, mResetInfo->reason, mResetInfo->exccause);

      // 83 chars of epc1, epc2, epc3, excvaddr, depc info + 13 chars of >stack>
      numBytesWritten += sprintf(tmpBuffer + numBytesWritten, "epc1=0x%08x epc2=0x%08x epc3=0x%08x excvaddr=0x%08x depc=0x%08x\n>>>stack>>>\n", mResetInfo->epc1, mResetInfo->epc2, mResetInfo->epc3, mResetInfo->excvaddr, mResetInfo->depc);

      uint32_t stackTrace = 0;

      // collect stack trace
      // one loop contains 45 chars of stack address and its content
      // e.g. "3fffffb0: feefeffe feefeffe 3ffe8508 40100459"
      for (uint16_t i = 0; i < stackLength; i += 0x10)
      {
        numBytesWritten += sprintf(tmpBuffer + numBytesWritten, "%08x: ", mStackStart + i);

        for (uint8_t j = 0; j < 4; j++)
        {
          uint32_t* byteptr = (uint32_t*) (mStackStart + i + j*4);
          stackTrace = *byteptr;

          numBytesWritten += sprintf(tmpBuffer + numBytesWritten, "%08x ", stackTrace);
        }
      }
      // as we checked +15 char, this will always fit
      numBytesWritten += sprintf(tmpBuffer + numBytesWritten, "%s", "<<<stack<<<\n\n");

      Bytes dumpData((const char*)tmpBuffer);
      free(tmpBuffer);
      return dumpData;
    }

    friend void uniotCrashCallback(struct rst_info *resetInfo, uint32_t stackStart, uint32_t stackEnd);

    struct rst_info *mResetInfo;
    uint32_t mStackStart;
    uint32_t mStackEnd;
  };
} // namespace uniot


