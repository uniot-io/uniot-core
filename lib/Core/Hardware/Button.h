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
#include <LinkRegisterRecord.h>

namespace uniot
{
class Button : public LinkRegisterRecord
{
public:
  enum Event
  {
    CLICK,
    LONG_PRESS
  };
  using ButtonCallback = std::function<void(Button *, Event)>;
  Button(uint8_t pin, uint8_t activeLevel, uint8_t longPressTicks, ButtonCallback commonCallback = nullptr, uint8_t autoResetTicks = 100)
      : LinkRegisterRecord(),
        mPin(pin),
        mActiveLevel(activeLevel),
        mLongPressTicks(longPressTicks),
        mAutoResetTicks(autoResetTicks),
        OnLongPress(commonCallback),
        OnClick(commonCallback),
        mPrevState(false),
        mLongPressTicker(0),
        mAutoResetTicker(0),
        mTaskCallback([this](short t) {
          bool curState = digitalRead(mPin) == mActiveLevel;
          if (curState && ++mLongPressTicker == mLongPressTicks)
          {
            mWasLongPress = true;
            if (OnLongPress)
              OnLongPress(this, LONG_PRESS);
          }
          if (mPrevState && !curState)
          {
            if (mLongPressTicker < mLongPressTicks)
            {
              mWasClick = true;
              if (OnClick)
                OnClick(this, CLICK);
            }
            mLongPressTicker = 0;
            mAutoResetTicker = 0;
          }
          mPrevState = curState;

          if (++mAutoResetTicker == mAutoResetTicks)
          {
            resetClick();
            resetLongPress();
            mAutoResetTicker = 0;
          }
        })
  {
    pinMode(mPin, INPUT);
  }

  uniot::SchedulerTask::SchedulerTaskCallback getTaskCallback()
  {
    return mTaskCallback;
  }

  bool resetClick()
  {
    auto was = mWasClick;
    mWasClick = false;
    return was;
  }

  bool resetLongPress()
  {
    auto was = mWasLongPress;
    mWasClick = false;
    return was;
  }

protected:
  uint8_t mPin;
  uint8_t mActiveLevel;

  uint8_t mLongPressTicks;
  uint8_t mAutoResetTicks;

  bool mWasClick;
  bool mWasLongPress;

  ButtonCallback OnLongPress;
  ButtonCallback OnClick;

  bool mPrevState;
  uint8_t mLongPressTicker;
  uint8_t mAutoResetTicker;

  SchedulerTask::SchedulerTaskCallback mTaskCallback;
};
} // namespace uniot
