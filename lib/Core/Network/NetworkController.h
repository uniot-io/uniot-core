/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2025 Uniot <contact@uniot.io>
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

#include <Button.h>
#include <CBORStorage.h>
#include <EventListener.h>
#include <ISchedulerConnectionKit.h>
#include <NetworkEvents.h>
#include <NetworkScheduler.h>

/**
 * @file NetworkController.h
 * @brief High-level network management controller with user interface
 * @defgroup network_controller Network Controller
 * @ingroup network
 * @{
 *
 * This file provides a comprehensive network management controller that combines
 * network connectivity with user interface elements. The NetworkController acts
 * as a bridge between the NetworkScheduler and physical hardware components like
 * buttons and LEDs, providing intuitive device control and status indication.
 *
 * Key features:
 * - Button-based network configuration control
 * - LED status indication for connection states
 * - Automatic network recovery and reconnection
 * - Persistent storage of device state
 * - Reboot detection and handling for configuration reset
 * - Event-driven architecture with automatic state management
 *
 * The controller implements intelligent behavior patterns:
 * - Single click: Manual reconnection attempt
 * - Long press: Configuration reset (if multiple clicks precede)
 * - Multiple reboots: Automatic configuration reset
 * - LED patterns: Visual feedback for different network states
 *
 * Example usage:
 * @code
 * Credentials credentials;
 * NetworkScheduler networkScheduler(credentials);
 * NetworkController controller(networkScheduler,
 *                             BUTTON_PIN, LOW,    // Button on pin with active LOW
 *                             LED_PIN, HIGH);     // LED on pin with active HIGH
 *
 * TaskScheduler scheduler;
 * controller.pushTo(scheduler);
 * networkScheduler.pushTo(scheduler);
 *
 * controller.attach();
 * networkScheduler.attach();
 * @endcode
 */

namespace uniot {
/**
 * @brief Network management controller with user interface integration
 *
 * This class provides a complete network management solution that combines
 * the NetworkScheduler with physical user interface elements. It handles
 * button interactions for network configuration, LED status indication,
 * automatic recovery mechanisms, and persistent state management.
 *
 * The controller operates as a finite state machine that responds to both
 * network events and user input, providing intelligent behavior patterns
 * for device configuration and management. It implements multiple fallback
 * mechanisms to ensure devices remain configurable even after connection
 * failures or repeated reboots.
 *
 * State management includes:
 * - Connection status tracking and LED indication
 * - Button click counting for advanced configuration modes
 * - Reboot counting for automatic configuration reset
 * - Persistent storage of critical state information
 */
class NetworkController : public ISchedulerConnectionKit, public CoreEventListener, public CBORStorage {
 public:
  /**
   * @brief Construct a new NetworkController
   * @param network Reference to the NetworkScheduler to control
   * @param pinBtn GPIO pin for configuration button (UINT8_MAX to disable)
   * @param activeLevelBtn Active logic level for button (LOW or HIGH)
   * @param pinLed GPIO pin for status LED (UINT8_MAX to disable)
   * @param activeLevelLed Active logic level for LED (LOW or HIGH)
   * @param maxRebootCount Maximum reboots before auto-reset (default: 3)
   * @param rebootWindowMs Time window for reboot counting in milliseconds (default: 10000)
   *
   * Initializes the network controller with specified hardware configuration.
   * The controller will automatically handle GPIO configuration for enabled
   * components and set up event listeners for network state changes.
   *
   * Button behavior:
   * - Single click: Increment click counter
   * - Long press: Reconnect (normal) or reset configuration (if >3 clicks)
   * - Multiple rapid clicks enable advanced configuration modes
   */
  NetworkController(NetworkScheduler &network,
    uint8_t pinBtn = UINT8_MAX,
    uint8_t activeLevelBtn = LOW,
    uint8_t pinLed = UINT8_MAX,
    uint8_t activeLevelLed = HIGH,
    uint8_t maxRebootCount = 3,
    uint32_t rebootWindowMs = 10000)
      : CBORStorage("ctrl.cbor"),
        mpNetwork(&network),
        mNetworkLastState(events::network::Msg::SUCCESS),
        mClickCounter(0),
        mPinLed(pinLed),
        mActiveLevelLed(activeLevelLed),
        mMaxRebootCount(maxRebootCount),
        mRebootWindowMs(rebootWindowMs),
        mRebootCount(0) {
    if (pinLed != UINT8_MAX) {
      pinMode(mPinLed, OUTPUT);
    }
    if (pinBtn != UINT8_MAX) {
      mpConfigBtn = MakeUnique<Button>(pinBtn, activeLevelBtn, 30, [&](Button *btn, Button::Event event) {
        switch (event) {
          case Button::LONG_PRESS:
            if (mClickCounter > 3)
              mpNetwork->forget();
            else
              mpNetwork->reconnect();
            break;
          case Button::CLICK:
            if (!mClickCounter) {
              mpTaskResetClickCounter->attach(5000, 1);
            }
            mClickCounter++;
          default:
            break;
        }
      });
    }
    _initTasks();
    _checkAndHandleReboot();
    CoreEventListener::listenToEvent(events::network::Topic::CONNECTION);
  }

  /**
   * @brief Destroy the NetworkController
   *
   * Cleanly shuts down the controller by stopping event listening and
   * clearing network references to prevent dangling pointers.
   */
  virtual ~NetworkController() {
    CoreEventListener::stopListeningToEvent(events::network::Topic::CONNECTION);
    mpNetwork = nullptr;
  }

  /**
   * @brief Store controller state to persistent storage
   * @retval bool true if storage was successful, false otherwise
   *
   * Saves critical controller state including reboot counter to CBOR storage.
   * This enables reboot detection and automatic configuration reset behavior.
   */
  virtual bool store() override {
    object().put("reset", mRebootCount);
    return CBORStorage::store();
  }

  /**
   * @brief Restore controller state from persistent storage
   * @retval bool true if restoration was successful, false otherwise
   *
   * Loads previously saved controller state from CBOR storage, particularly
   * the reboot counter used for automatic configuration reset detection.
   */
  virtual bool restore() override {
    if (CBORStorage::restore()) {
      mRebootCount = object().getInt("reset");
      return true;
    }
    return false;
  }

  /**
   * @brief Handle network events and update controller state
   * @param topic Event topic identifier
   * @param msg Event message identifier
   *
   * Processes network events and implements the controller's state machine
   * logic for automatic network recovery, status indication, and user
   * interaction handling. Responds to connection state changes with
   * appropriate LED patterns and recovery actions.
   */
  virtual void onEventReceived(unsigned int topic, int msg) override {
    if (events::network::Topic::CONNECTION == topic) {
      int lastState = _resetNetworkLastState(msg);
      switch (msg) {
        case events::network::Msg::ACCESS_POINT:
          if (lastState != events::network::Msg::FAILED) {
            statusWaiting();
          }
          break;
        case events::network::Msg::SUCCESS:
          statusIdle();
          break;
        case events::network::Msg::CONNECTING:
          statusBusy();
          break;
        case events::network::Msg::DISCONNECTED:
          if (lastState != events::network::Msg::CONNECTING) {
            // If previous state was CONNECTING, most likely there was a manual reconnect request
            mpNetwork->reconnect();
          }
          break;
        case events::network::Msg::AVAILABLE:
          mpNetwork->reconnect();
          break;
        case events::network::Msg::FAILED:
          statusAlarm();
          mpNetwork->config();
          break;
        default:
          break;
      }
    }
  }

  /**
   * @brief Push all controller tasks to the scheduler
   * @param scheduler TaskScheduler instance to receive the tasks
   *
   * Registers all controller tasks including LED signaling, reboot counter
   * reset, button handling, and click counter reset. Only registers button-
   * related tasks if a button is configured.
   */
  virtual void pushTo(TaskScheduler &scheduler) override {
    scheduler.push("signal_led", mpTaskSignalLed);
    scheduler.push("rst_reboot_count", mpTaskResetRebootCounter);
    if (_hasButton()) {
      scheduler.push("btn_config", mpTaskConfigBtn);
      scheduler.push("rst_click_count", mpTaskResetClickCounter);
    }
  }

  /**
   * @brief Attach the controller and start initial operations
   *
   * Starts the reboot counter timer, button monitoring (if configured),
   * and sets initial status indication to busy state.
   */
  virtual void attach() override {
    mpTaskResetRebootCounter->once(mRebootWindowMs);
    if (_hasButton()) {
      mpTaskConfigBtn->attach(100);
    }
    statusBusy();
  }

  /**
   * @brief Set LED to waiting status pattern
   *
   * Configures LED to indicate waiting/configuration mode with 1-second
   * blinking pattern. Used when device is in access point mode waiting
   * for user configuration.
   */
  void statusWaiting() {
    mpTaskSignalLed->attach(1000);
  }

  /**
   * @brief Set LED to busy status pattern
   *
   * Configures LED to indicate busy/connecting state with 500ms blinking
   * pattern. Used when device is attempting network connection.
   */
  void statusBusy() {
    mpTaskSignalLed->attach(500);
  }

  /**
   * @brief Set LED to alarm status pattern
   *
   * Configures LED to indicate error/alarm state with fast 200ms blinking
   * pattern. Used when network connection has failed and requires attention.
   */
  void statusAlarm() {
    mpTaskSignalLed->attach(200);
  }

  /**
   * @brief Set LED to idle status pattern
   *
   * Configures LED to indicate successful connection with single brief flash.
   * Used when device is successfully connected to the network.
   */
  void statusIdle() {
    mpTaskSignalLed->attach(200, 1);
  }

  /**
   * @brief Get access to the configuration button
   * @retval Button* Pointer to button instance, or nullptr if no button configured
   *
   * Provides access to the button instance for external control or status
   * checking. Returns nullptr if no button was configured during construction.
   */
  Button *getButton() {
    return _hasButton() ? mpConfigBtn.get() : nullptr;
  }

 private:
  /**
   * @brief Initialize all controller tasks
   *
   * Creates and configures all scheduled tasks including LED signaling,
   * button handling, click counter reset, and reboot counter reset.
   * Only creates button-related tasks if a button is configured.
   */
  void _initTasks() {
    mpTaskSignalLed = TaskScheduler::make([&](SchedulerTask &self, short t) {
      static bool signalLevel = true;
      signalLevel = (!signalLevel && t);
      CoreEventListener::emitEvent(events::network::Topic::WIFI_STATUS_LED, signalLevel);

      if (_hasLed()) {
        digitalWrite(mPinLed, signalLevel ? mActiveLevelLed : !mActiveLevelLed);
      }
    });

    if (_hasButton()) {
      mpTaskConfigBtn = TaskScheduler::make(*mpConfigBtn);
      mpTaskResetClickCounter = TaskScheduler::make([&](SchedulerTask &self, short t) {
        UNIOT_LOG_DEBUG("ClickCounter = %d", mClickCounter);
        mClickCounter = 0;
      });
    }

    mpTaskResetRebootCounter = TaskScheduler::make([&](SchedulerTask &self, short t) {
      mRebootCount = 0;
      NetworkController::store();
    });
  }

  /**
   * @brief Check reboot count and handle automatic configuration reset
   *
   * Examines the stored reboot count and automatically triggers configuration
   * reset if the maximum reboot threshold is exceeded. This provides a
   * recovery mechanism for devices that become unconfigurable due to
   * network issues or corrupted settings.
   */
  void _checkAndHandleReboot() {
    NetworkController::restore();
    mRebootCount++;
    if (mRebootCount >= mMaxRebootCount) {
      mpTaskResetRebootCounter->detach();
      mpNetwork->forget();
      mRebootCount = 0;
    }
    NetworkController::store();
  }

  /**
   * @brief Update and return previous network state
   * @param newState New network state to set
   * @retval int Previous network state value
   *
   * Thread-safe method to update the network state while returning the
   * previous state. Used for state transition logic and preventing
   * redundant operations.
   */
  int _resetNetworkLastState(int newState) {
    auto oldState = mNetworkLastState;
    mNetworkLastState = newState;
    return oldState;
  }

  /**
   * @brief Check if controller has a configured button
   * @retval bool true if button is available, false otherwise
   */
  inline bool _hasButton() {
    return mpConfigBtn.get() != nullptr;
  }

  /**
   * @brief Check if controller has a configured LED
   * @retval bool true if LED is available, false otherwise
   */
  inline bool _hasLed() {
    return mPinLed != UINT8_MAX;
  }

  NetworkScheduler *mpNetwork;  ///< Pointer to the managed network scheduler
  int mNetworkLastState;        ///< Last recorded network state for transition detection

  uint8_t mClickCounter;     ///< Counter for rapid button clicks
  uint8_t mPinLed;           ///< GPIO pin number for status LED
  uint8_t mActiveLevelLed;   ///< Active logic level for LED (HIGH/LOW)
  uint8_t mMaxRebootCount;   ///< Maximum reboots before automatic reset
  uint32_t mRebootWindowMs;  ///< Time window for reboot counting
  uint8_t mRebootCount;      ///< Current reboot count within window

  UniquePointer<Button> mpConfigBtn;  ///< Smart pointer to configuration button instance

  // Task pointers for scheduled operations
  TaskScheduler::TaskPtr mpTaskSignalLed;           ///< Task for LED status signaling
  TaskScheduler::TaskPtr mpTaskConfigBtn;           ///< Task for button monitoring and handling
  TaskScheduler::TaskPtr mpTaskResetClickCounter;   ///< Task for resetting button click counter
  TaskScheduler::TaskPtr mpTaskResetRebootCounter;  ///< Task for resetting reboot counter
};
}  // namespace uniot

/** @} */
