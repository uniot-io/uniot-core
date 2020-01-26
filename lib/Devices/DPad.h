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

class DPad {
public:
  DPad(byte up, byte center, byte down, byte actState = LOW, byte pinMode = INPUT) 
  : mUp(up), mCenter(center), mDown(down), mActState(actState), mPinMode(pinMode) {}
  
  void init(void) {
    pinMode(mUp, mPinMode);
    pinMode(mCenter, mPinMode);
    pinMode(mDown, mPinMode);
  }

  byte isUp(void){
    return isAct(mUp);
  }

  byte isCenter(void){
    return isAct(mCenter);
  }

  byte isDown(void){
    return isAct(mDown);
  }

private:
  byte isAct(byte btn) {
    return digitalRead(btn) == mActState;
  }
  
  byte mUp, mCenter, mDown, mActState, mPinMode;
};
