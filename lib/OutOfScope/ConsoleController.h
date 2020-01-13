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

#include <ILightPrint.h>
#include <IExecutor.h>
#include <ClearQueue.h>
#include <ConsoleView.h>

class ConsoleController : public uniot::IExecutor, public uniot::ILightPrint
{
public:
  ConsoleController(ConsoleView* display);
  virtual void println(const char* str) override;
  virtual void printlnPGM(const char* PSTR_str) override;
  void printlnString(const String& str);
  virtual uint8_t execute() override;

private:
  struct MemStr
  {
    enum Type { NONE = 0, STR, PSTR };

    Type strType;
    const char* strPtr;
    String* strString;        //unique_ptr takes more memory

    void make(const MemStr& memStr) {
      strString = memStr.strString ? new String(memStr.strString->c_str()) : nullptr;
      strPtr = strString ? strString->c_str() : memStr.strPtr;
      strType = memStr.strType;
    }

    MemStr(const char* str, Type type)
    : strString(nullptr), strPtr(str), strType(type) {}
    MemStr()
    : MemStr(nullptr, NONE) {}
    MemStr(const String& str)
    : strType(MemStr::STR) {
      strString = new String(str.c_str());
      strPtr = strString->c_str();
    }
    MemStr(const MemStr& memStr) {
      make(memStr);
    }
    ~MemStr() {
      if(strString) {
        delete strString;
        strString = nullptr;
      }
      strPtr = nullptr;
    }

    MemStr & operator =(const MemStr& memStr) {
      if(this == &memStr) {
        return *this;
      }
      make(memStr);
      return *this;
    }
  };

  ConsoleView* mpConsole;
  ClearQueue<MemStr> mStrQueue;
};

ConsoleController::ConsoleController(ConsoleView* console): mpConsole(console) {
}

void ConsoleController::println(const char* str) {
  mStrQueue.push(MemStr(str, MemStr::STR));
}

void ConsoleController::printlnPGM(const char* PSTR_str) {
  mStrQueue.push(MemStr(PSTR_str, MemStr::PSTR));
}

void ConsoleController::printlnString(const String& str) {
  mStrQueue.push(MemStr(str));
}

uint8_t ConsoleController::execute() {
  if(!mStrQueue.isEmpty()) {
    while(!mStrQueue.isEmpty()) {
      auto memStr = mStrQueue.pop(MemStr());
      switch(memStr.strType) {
        case MemStr::STR: mpConsole->println(memStr.strPtr); break;
        case MemStr::PSTR: mpConsole->printlnPGM(memStr.strPtr); break;
        default: break;
      }
    }
    mpConsole->redraw();
  }
  return 0;
}
