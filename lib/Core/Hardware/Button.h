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

/** @cond */
/**
 * DO NOT DELETE THE "harware" GROUP DEFINITION BELOW.
 * Used to create the Hardware topic in the documentation. If you want to delete this file,
 * please paste the group definition into another utility and delete this one.
 */
/** @endcond */

/**
 * @defgroup hardware Hardware
 * @brief Hardware-related components and functions for the Uniot Core
 *
 * This collection offers reusable hardware components optimized for embedded systems
 * in the Uniot Core. These components are designed with minimal
 * memory footprint and efficient performance, specifically targeting IoT
 * and resource-constrained embedded applications.
 */

#pragma once

#include <IExecutor.h>
#include <ObjectRegisterRecord.h>
#include <TaskScheduler.h>

namespace uniot {
/**
 * @brief Button input handler with support for click and long press detection.
 * @defgroup hardware_button Button
 * @ingroup hardware
 * @{
 *
 * The Button class provides functionality for detecting button presses and distinguishing
 * between regular clicks and long presses. It implements the IExecutor interface for
 * integration with task scheduling systems and ObjectRegisterRecord for using in a registry system.
 */
class Button : public IExecutor, public ObjectRegisterRecord {
 public:
  /**
   * @enum Event
   * @brief Defines types of button events that can be triggered.
   */
  enum Event {
    CLICK,      ///< Regular short button press and release
    LONG_PRESS  ///< Button held down for longer than the defined threshold
  };

  /**
   * @typedef ButtonCallback
   * @brief Callback function signature for button events.
   *
   * @param button Pointer to the Button instance that triggered the event
   * @param event The type of event that occurred (CLICK or LONG_PRESS)
   */
  using ButtonCallback = std::function<void(Button *, Event)>;

  /**
   * @brief Constructs a Button instance.
   *
   * @param pin GPIO pin number connected to the button
   * @param activeLevel Logic level that represents button press (HIGH or LOW)
   * @param longPressTicks Number of execution ticks required for long press detection
   * @param commonCallback Optional callback function for both click and long press events
   * @param autoResetTicks Number of ticks after which button state will automatically reset
   */
  Button(uint8_t pin, uint8_t activeLevel, uint8_t longPressTicks, ButtonCallback commonCallback = nullptr, uint8_t autoResetTicks = 100)
      : ObjectRegisterRecord(),
        mPin(pin),
        mActiveLevel(activeLevel),
        mLongPressTicks(longPressTicks),
        mAutoResetTicks(autoResetTicks),
        mWasClick(false),
        mWasLongPress(false),
        OnLongPress(commonCallback),
        OnClick(commonCallback),
        mPrevState(false),
        mLongPressTicker(0),
        mAutoResetTicker(0) {
    pinMode(mPin, INPUT);
  }

  /**
   * @brief Resets the click detection flag.
   *
   * @retval true Click event was detected since the last reset
   * @retval false No click event was detected since the last reset
   */
  bool resetClick() {
    auto was = mWasClick;
    mWasClick = false;
    return was;
  }

  /**
   * @brief Resets the long press detection flag.
   *
   * @retval true Long press event was detected since the last reset
   * @retval false No long press event was detected since the last reset
   */
  bool resetLongPress() {
    auto was = mWasLongPress;
    mWasLongPress = false;
    return was;
  }

  /**
   * @brief Processes button state and detects events as part of the scheduler system.
   *
   * This method is called periodically by the TaskScheduler and implements the IExecutor
   * interface. Each time it's called, it:
   * 1. Reads the current button state
   * 2. Detects if a long press has occurred
   * 3. Detects if a click has occurred (on button release)
   * 4. Triggers appropriate callbacks when events are detected
   * 5. Manages the auto-reset functionality to clear event flags
   *
   * @param times The number of remaining executions as passed from the scheduler:
   *        - Positive numbers indicate remaining executions count
   *        - Negative numbers indicate infinite executions (repeat forever)
   *        - Zero indicates the last execution has occurred
   */
  virtual void execute(short times) override {
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
  }

  /**
   * @brief Returns the type identifier for this class.
   *
   * @retval type_id The type identifier for the Button class
   */
  virtual type_id getTypeId() const override {
    return Type::getTypeId<Button>();
  }

 protected:
  uint8_t mPin;             ///< GPIO pin number connected to the button
  uint8_t mActiveLevel;     ///< Logic level that represents button press (HIGH or LOW)

  uint8_t mLongPressTicks;  ///< Number of ticks to define a long press
  uint8_t mAutoResetTicks;  ///< Number of ticks after which button state auto-resets

  bool mWasClick;           ///< Flag indicating if a click has been detected
  bool mWasLongPress;       ///< Flag indicating if a long press has been detected

  ButtonCallback OnLongPress; ///< Callback function for long press events
  ButtonCallback OnClick;     ///< Callback function for click events

  bool mPrevState;          ///< Previous state of the button
  uint8_t mLongPressTicker; ///< Counter for tracking potential long presses
  uint8_t mAutoResetTicker; ///< Counter for automatic state reset
};
/** @} */ // End of hardware_button group
}  // namespace uniot
