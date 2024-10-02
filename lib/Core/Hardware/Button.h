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

#include <IExecutor.h>
#include <ObjectRegisterRecord.h>
#include <TaskScheduler.h>

namespace uniot {
class Button : public IExecutor, public ObjectRegisterRecord {
 public:
  enum Event {
    CLICK,
    LONG_PRESS
  };
  using ButtonCallback = std::function<void(Button *, Event)>;
  Button(uint8_t pin, uint8_t activeLevel, uint8_t longPressTicks, ButtonCallback commonCallback = nullptr, uint8_t autoResetTicks = 100)
      : ObjectRegisterRecord(),
        mPin(pin),
        mActiveLevel(activeLevel),
        mLongPressTicks(longPressTicks),
        mAutoResetTicks(autoResetTicks),
        OnLongPress(commonCallback),
        OnClick(commonCallback),
        mPrevState(false),
        mLongPressTicker(0),
        mAutoResetTicker(0) {
    pinMode(mPin, INPUT);
  }

  bool resetClick() {
    auto was = mWasClick;
    mWasClick = false;
    return was;
  }

  bool resetLongPress() {
    auto was = mWasLongPress;
    mWasLongPress = false;
    return was;
  }

  virtual uint8_t execute() override {
    bool curState = digitalRead(mPin) == mActiveLevel;
    if (curState && ++mLongPressTicker == mLongPressTicks) {
      mWasLongPress = true;
      if (OnLongPress)
        OnLongPress(this, LONG_PRESS);
    }
    if (mPrevState && !curState) {
      if (mLongPressTicker < mLongPressTicks) {
        mWasClick = true;
        if (OnClick)
          OnClick(this, CLICK);
      }
      mLongPressTicker = 0;
      mAutoResetTicker = 0;
    }
    mPrevState = curState;

    if (++mAutoResetTicker == mAutoResetTicks) {
      resetClick();
      resetLongPress();
      mAutoResetTicker = 0;
    }

    return 0;
  }

  virtual type_id getTypeId() const override {
    return Type::getTypeId<Button>();
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
};
}  // namespace uniot
