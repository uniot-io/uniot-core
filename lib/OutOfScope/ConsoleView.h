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
#include <i2c_ssd1306.h>

// #define SSD_1306_64x48

#if defined (SSD_1306_64x48)
#define ROW_MAX             16
#define ROW_FIT_OFFSET      2
#define ROW_FIT_COUNT       6
#define CHARS_IN_ROW        8
#define PX_BEGIN_OFFSET     31
#else
#define ROW_MAX             16
#define ROW_FIT_OFFSET      0
#define ROW_FIT_COUNT       8   // HEIGHT/FONT_SIZE
#define CHARS_IN_ROW        16  // WIDTH/FONT_SIZE
#define PX_BEGIN_OFFSET     0
#endif

class ConsoleView : public uniot::ILightPrint 
{
public:
  ConsoleView(SSD1306* display);
  void draw(void);
  void redraw(void);
  void incRow(void);
  void decRow(void);
  virtual void println(const char* str);
  virtual void printlnPGM(const char* PSTR_str);
  void clear(void);

private:
  void clearScreen(void);
  void addLineToScreen(void);
  void execLine(void);
  void charToLine(uint8_t c);

  SSD1306* mpDisplay;
  uint8_t mCurRowBuf[CHARS_IN_ROW + 1]; // + \0
  uint8_t mCurRowCharPos = 0;
  uint8_t mScreen[ROW_MAX][CHARS_IN_ROW + 1]; // + \0
  uint8_t mRealRowCount, mCurRowPos;
};
