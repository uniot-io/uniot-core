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

#if defined(ESP32)
#include <driver/gpio.h>
#endif

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
    applyPinMode();
  }

  /**
   * @brief Resets the click detection flag.
   *
   * @retval true Click event was detected since the last reset
   * @retval false No click event was detected since the last reset
   */
  /**
   * @brief Configure the pin as an input with the pull its active level needs.
   *
   * A button that is active LOW must read HIGH when released, so it gets a pull-up; one that is
   * active HIGH gets a pull-down. The internal resistor is used where the pin has one, and an
   * external resistor on the board simply works alongside it. Where the pin has no suitable
   * internal pull -- ESP8266 GPIO0-15 have no pull-down and GPIO16 no pull-up, classic ESP32
   * GPIO34-39 have neither -- the pin is set to plain INPUT and an external resistor is needed.
   *
   * Called by the constructor, and again for every Lisp button by UniotCore::begin(). Calling
   * pinMode() on the pin after begin() overrides it.
   */
  void applyPinMode() {
    pinMode(mPin, _inputMode(mPin, mActiveLevel));
  }

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
    // The counter stops at the threshold. It is a uint8_t, so left running it would wrap
    // after 255 ticks: a long hold would fire LONG_PRESS again, and releasing just after the
    // wrap would count as a click.
    if (curState && mLongPressTicker < mLongPressTicks) {
      if (++mLongPressTicker == mLongPressTicks) {
        mWasLongPress = true;
        if (OnLongPress)
          OnLongPress(this, LONG_PRESS);
      }
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
  /**
   * @brief The input mode for a pin, with the internal pull matching the active level.
   *
   * @param pin GPIO pin number
   * @param activeLevel Logic level while pressed (LOW or HIGH)
   * @retval uint8_t A mode for pinMode()
   */
  static uint8_t _inputMode(uint8_t pin, uint8_t activeLevel) {
#if defined(ESP8266)
    if (pin == 16) {
      return activeLevel == HIGH ? INPUT_PULLDOWN_16 : INPUT;
    }
    return activeLevel == LOW ? INPUT_PULLUP : INPUT;
#elif defined(ESP32)
    // Input-only pins have no pull resistors, and asking for one can fail the whole config.
    if (!GPIO_IS_VALID_OUTPUT_GPIO(pin)) {
      return INPUT;
    }
    return activeLevel == LOW ? INPUT_PULLUP : INPUT_PULLDOWN;
#else
    return INPUT;
#endif
  }

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
