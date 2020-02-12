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

#include <TaskScheduler.h>

namespace uniot
{
class Button
{
public:
  enum Event { CLICK, LONG_PRESS };
  using ButtonCallback = std::function<void(Button*, Event)>;
  Button(uint8_t pin, uint8_t activeLevel, uint8_t longPressTicks, ButtonCallback commonCallback = nullptr) 
  : mPin(pin),
  mActiveLevel(activeLevel),
  mLongPressTicks(longPressTicks),
  OnLongPress(commonCallback),
  OnClick(commonCallback),
  mTaskCallback([this](short t) {
    static bool prevState = false;
    static uint8_t ticker = 0;
    bool curState = digitalRead(mPin) == mActiveLevel;
    if(curState && ++ticker == mLongPressTicks) {
      if(OnLongPress) {
        OnLongPress(this, LONG_PRESS);
      }
    }
    if(prevState && !curState) {
      if(ticker < mLongPressTicks && OnClick) {
        OnClick(this, CLICK);
      } 
      ticker = 0;
    }
    prevState = curState;
  }) { 
    pinMode(mPin, INPUT);
  }

  uniot::SchedulerTask::SchedulerTaskCallback getTaskCallback() {
    return mTaskCallback;
  }

protected:
  uint8_t mPin;
  uint8_t mActiveLevel;
  uint8_t mLongPressTicks;

  ButtonCallback OnLongPress;
  ButtonCallback OnClick;

  SchedulerTask::SchedulerTaskCallback mTaskCallback;
};
} // namespace uniot
