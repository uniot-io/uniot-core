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

#include <AppKit.h>
#include <Arduino.h>
#include <ClearQueue.h>
#include <Credentials.h>
#include <Date.h>
#include <EventBus.h>
#include <Logger.h>
#include <TaskScheduler.h>

/**
 * @file Uniot.h
 * @brief Main API interface for the Uniot IoT platform
 * @defgroup uniot_core Uniot Core
 * @ingroup core
 * @{
 *
 * This file provides the primary interface for the Uniot IoT platform, offering
 * a simplified API for developing embedded applications. The UniotCore class
 * abstracts the complexity of the underlying subsystems and provides intuitive
 * methods for common IoT operations.
 *
 * Key features:
 * - WiFi network management with automatic recovery
 * - Task scheduling and timer management
 * - Event-driven communication system
 * - Embedded Lisp scripting capabilities
 * - GPIO management and hardware abstraction
 * - Time synchronization and persistence
 * - Comprehensive logging and debugging
 *
 * The API is designed to be familiar to web developers while providing the
 * performance and reliability required for embedded systems. It follows
 * patterns similar to Node.js for timer management and event handling.
 *
 * Example usage:
 * @code
 * #include <Uniot.h>
 *
 * void setup() {
 *   Uniot.configWiFiCredentials("MyNetwork", "password");
 *   Uniot.configWiFiStatusLed(LED_BUILTIN);
 *   Uniot.configWiFiResetButton(0, LOW);
 *
 *   Uniot.begin();
 * }
 *
 * void loop() {
 *   Uniot.loop();
 * }
 * @endcode
 */

/**
 * @brief Main API class for the Uniot IoT platform
 *
 * This class provides a comprehensive interface for building IoT applications
 * with minimal boilerplate code. It orchestrates all platform subsystems
 * including networking, scheduling, scripting, and hardware management.
 *
 * The UniotCore class implements a singleton-like pattern through the global
 * `Uniot` instance, providing immediate access to platform capabilities
 * without complex initialization procedures.
 *
 * Architecture overview:
 * - Task scheduler: Non-blocking execution of periodic and one-shot tasks
 * - Event bus: Decoupled communication between system components
 * - Network stack: Automatic WiFi management with fallback mechanisms
 * - Lisp interpreter: Dynamic scripting and runtime reconfiguration
 * - Hardware abstraction: GPIO management and peripheral control
 */
class UniotCore {
 public:
  using TimerId = uint32_t;     ///< Type for timer identifiers
  using ListenerId = uint32_t;  ///< Type for event listener identifiers

  static constexpr TimerId INVALID_TIMER_ID = 0;        ///< Invalid timer ID constant
  static constexpr ListenerId INVALID_LISTENER_ID = 0;  ///< Invalid listener ID constant

  /**
   * @brief Construct a new UniotCore instance
   *
   * Initializes the core platform components including the task scheduler
   * and event bus. Network controller configuration is deferred until
   * begin() is called to allow for runtime configuration.
   */
  UniotCore() : mScheduler(), mEventBus(FOURCC(main)), mpNetworkControllerConfig(nullptr) {}

  /**
   * @brief Configure WiFi reset button
   * @param pinBtn GPIO pin number for the reset button
   * @param activeLevelBtn Active logic level for button (LOW or HIGH)
   * @param registerLispBtn Whether to register button with Lisp interpreter
   *
   * Configures a hardware button for WiFi network reset functionality.
   * The button provides both manual reconnection (single press) and
   * configuration reset (multiple rapid presses followed by long press).
   *
   * With registerLispBtn, the button is linked into the bclicked register during begin().
   * Buttons added with addLispButton() before begin() therefore come before it; add them after
   * begin(), still in setup(), to keep this button at index 0.
   */
  void configWiFiResetButton(uint8_t pinBtn, uint8_t activeLevelBtn = LOW, bool registerLispBtn = true) {
    _createNetworkControllerConfig();
    mpNetworkControllerConfig->pinBtn = pinBtn;
    mpNetworkControllerConfig->activeLevelBtn = activeLevelBtn;
    mpNetworkControllerConfig->registerLispBtn = registerLispBtn;
  }

  /**
   * @brief Configure WiFi status LED
   * @param pinLed GPIO pin number for the status LED
   * @param activeLevelLed Active logic level for LED (LOW or HIGH)
   *
   * Configures a hardware LED for visual indication of WiFi connection status.
   * The LED provides different blinking patterns for various connection states:
   * - Fast blink: Error/alarm state
   * - Medium blink: Connecting/busy state
   * - Slow blink: Waiting/configuration mode
   * - Brief flash: Connected/idle state
   */
  void configWiFiStatusLed(uint8_t pinLed, uint8_t activeLevelLed = HIGH) {
    _createNetworkControllerConfig();
    mpNetworkControllerConfig->pinLed = pinLed;
    mpNetworkControllerConfig->activeLevelLed = activeLevelLed;
  }

  /**
   * @brief Sentinel value for configWiFiResetOnReboot(): disables reboot-reset entirely.
   *
   * Pass this as maxRebootCount to opt out of the automatic configuration-reset
   * mechanism. No reboot counts are stored, read, or compared, and the device
   * will never be force-reset due to repeated reboots.
   *
   * @code
   * Uniot.configWiFiResetOnReboot(UniotCore::REBOOT_RESET_DISABLED);
   * @endcode
   */
  static constexpr uint8_t REBOOT_RESET_DISABLED = UINT8_MAX;

  /**
   * @brief Configure automatic WiFi reset on repeated reboots
   * @param maxRebootCount Maximum reboots before triggering configuration reset,
   *                       or UniotCore::REBOOT_RESET_DISABLED to opt out entirely.
   *                       Defaults to UNIOT_WIFI_REBOOT_RESET_COUNT.
   * @param rebootWindowMs Time window for counting reboots in milliseconds.
   *                       Defaults to UNIOT_WIFI_REBOOT_WINDOW_MS. Ignored when
   *                       maxRebootCount is REBOOT_RESET_DISABLED.
   *
   * Enables automatic configuration reset when the device reboots repeatedly
   * within a specified time window. This provides a recovery mechanism for
   * devices that become unreachable due to network configuration issues.
   *
   * Pass REBOOT_RESET_DISABLED to completely suppress this behaviour: no reboot
   * counter is stored to or read from flash, and the network configuration is
   * never reset automatically.
   *
   * Called with no arguments it is also the way to ask for a network controller on
   * a device that configures neither a button nor a status LED: the controller is
   * what emits the WiFi status events, and on such a device reboot-reset is the
   * only path back from bad credentials.
   */
  void configWiFiResetOnReboot(uint8_t maxRebootCount = UNIOT_WIFI_REBOOT_RESET_COUNT, uint32_t rebootWindowMs = UNIOT_WIFI_REBOOT_WINDOW_MS) {
    _createNetworkControllerConfig();
    mpNetworkControllerConfig->maxRebootCount = maxRebootCount;
    mpNetworkControllerConfig->rebootWindowMs = rebootWindowMs;
  }

  /**
   * @brief Configure WiFi network credentials
   * @param ssid Network SSID to connect to
   * @param password Network password (empty for open networks)
   *
   * Sets the WiFi credentials for automatic network connection. The credentials
   * are validated and stored persistently for use across device reboots.
   */
  void configWiFiCredentials(const String& ssid, const String& password = "") {
    auto success = getAppKit().setWiFiCredentials(ssid, password);
    UNIOT_LOG_ERROR_IF(!success, "Failed to set WiFi credentials");
  }

  /**
   * @brief Configure user identification
   * @param user User identifier string for device association
   *
   * Sets the user ID for device identification and association purposes.
   * This identifier is used for device management and tracking within
   * the Uniot ecosystem.
   */
  void configUser(const String& user) {
    auto success = getAppKit().setUserId(user);
    UNIOT_LOG_ERROR_IF(!success, "Failed to set user ID");
  }

  /**
   * @brief Register a custom gzipped page on the configuration portal
   * @param path URL path to serve the page on (e.g. "/app")
   * @param gzData Pointer to the gzipped page data
   * @param gzLen Length of the gzipped data in bytes
   * @param label Button label shown in the portal UI (empty hides the button)
   * @param contentType MIME content type (default: "text/html")
   *
   * Serves a user-supplied embedded page from the captive portal server, and
   * reports it to the configuration UI so it can link to the page.
   * Must be called before begin().
   *
   * Typically used with a C array generated from a gzipped HTML file:
   * @code
   * #include "my_page.html.gz.h"
   * Uniot.addCustomPage("/app", MY_PAGE_HTML_GZ, MY_PAGE_HTML_GZ_LENGTH, "My Settings");
   * @endcode
   */
  void addCustomPage(const String& path, const uint8_t* gzData, size_t gzLen, const String& label = "", const char* contentType = "text/html") {
    auto success = getAppKit().addCustomPage(path, gzData, gzLen, label, contentType);
    UNIOT_LOG_ERROR_IF(!success, "Failed to register custom page at %s", path.c_str());
  }

  /**
   * @brief Register a custom HTTP GET route on the configuration portal
   * @param path URL path for the route (e.g. "/api/data")
   * @param handler Callback invoked to handle the request
   *
   * Suitable for sensor data APIs and other endpoints that should not appear
   * as a navigation button. Must be called before begin().
   */
  void addCustomRoute(const String& path, ArRequestHandlerFunction handler) {
    auto success = getAppKit().addCustomRoute(path, HTTP_GET, handler);
    UNIOT_LOG_ERROR_IF(!success, "Failed to register custom route at %s", path.c_str());
  }

  /**
   * @brief Register a custom HTTP route on the configuration portal
   * @param path URL path for the route (e.g. "/api/data")
   * @param method HTTP method(s) to accept (e.g. HTTP_GET, HTTP_POST, HTTP_ANY)
   * @param handler Callback invoked to handle the request
   *
   * Must be called before begin().
   */
  void addCustomRoute(const String& path, WebRequestMethodComposite method, ArRequestHandlerFunction handler) {
    auto success = getAppKit().addCustomRoute(path, method, handler);
    UNIOT_LOG_ERROR_IF(!success, "Failed to register custom route at %s", path.c_str());
  }

  /**
   * @brief Register a user-defined MQTT device with the platform
   * @param device The MQTTDevice instance to register
   *
   * Adds the device to the MQTT subsystem and synchronizes its subscriptions,
   * so it can publish to and receive from its own topics.
   *
   * @code
   * MyDevice myDevice;
   * Uniot.addMQTTDevice(myDevice);
   * @endcode
   */
  void addMQTTDevice(uniot::MQTTDevice& device) {
    getAppKit().getMQTT().addDevice(device);
    device.syncSubscriptions();
  }

  /**
   * @brief Enable periodic saving of date/time information
   * @param periodSeconds Interval between saves in seconds (0 to disable)
   *
   * Configures automatic persistence of date/time information to prevent
   * time loss during device reboots. Useful for maintaining accurate
   * timestamps when NTP synchronization is not always available.
   */
  void enablePeriodicDateSave(uint32_t periodSeconds = 5 * 60) {
    if (periodSeconds > 0) {
      auto taskStoreDate = uniot::TaskScheduler::make(uniot::Date::getInstance());
      mScheduler.push("store_date", taskStoreDate);
      taskStoreDate->attach(periodSeconds * 1000L);
    }
  }

  /**
   * @brief Add a custom primitive to the Lisp interpreter
   * @param primitive Pointer to the primitive implementation
   *
   * Extends the embedded Lisp interpreter with custom functionality.
   * Primitives provide the bridge between Lisp scripts and hardware
   * or application-specific operations.
   */
  void addLispPrimitive(Primitive* primitive) {
    getAppKit().getLisp().pushPrimitive(primitive);
  }

  /**
   * @brief Set event interceptor for Lisp interpreter
   * @param interceptor Function to intercept and process Lisp events
   *
   * Configures a callback function to intercept events generated by
   * the Lisp interpreter, enabling custom event processing and
   * application-specific behavior.
   */
  void setLispEventInterceptor(uniot::LispEventInterceptor interceptor) {
    getAppKit().setLispEventInterceptor(interceptor);
  }

  /**
   * @brief Set the hook called when a Lisp script starts
   * @param hook Callback receiving why the script started
   *
   * Runs after the interpreter is built and its primitives are registered, but before
   * the script is evaluated, so a peripheral brought up here is ready for the first
   * pass. The reason says where the script came from: LispStartReason::Restored for one
   * loaded from flash at boot, LispStartReason::Received for one delivered over MQTT.
   *
   * @code
   * Uniot.setLispStartHook([](uniot::LispStartReason reason) {
   *   sensor.wake();
   * });
   * @endcode
   *
   * Keep the hook short -- see setLispStopHook() for why that matters most there.
   */
  void setLispStartHook(uniot::LispStartHook hook) {
    getAppKit().setLispStartHook(hook);
  }

  /**
   * @brief Set the hook called when a Lisp script stops
   * @param hook Callback receiving why the script stopped
   *
   * Runs before the interpreter is destroyed, whichever way the script ended:
   * Completed (it ran to the end, or a finite task used up its passes), Replaced (a new
   * script took its place), Cleared (an empty script arrived, meaning run nothing) or
   * Failed (a Lisp error tore the machine down).
   *
   * @code
   * Uniot.setLispStopHook([](uniot::LispStopReason reason) {
   *   sensor.sleep();   // short, and safe to run on any path
   * });
   * @endcode
   *
   * Both hooks run synchronously, so keep them short and move heavy work out with
   * setImmediate(), which runs it on the next scheduler pass instead:
   *
   * @code
   * Uniot.setLispStopHook([](uniot::LispStopReason reason) {
   *   sensor.sleep();
   *   Uniot.setImmediate([]() { report(); });   // the slow part, off this stack
   * });
   * @endcode
   *
   * This matters most for LispStopReason::Failed. That case is reached from the
   * interpreter's error printer while lisp_eval() is still unwinding, on a stack
   * already near the limit UNIOT_LISP_MAX_EVAL_STACK guards -- do only what is needed
   * to leave the hardware in a safe state, and defer everything else.
   */
  void setLispStopHook(uniot::LispStopHook hook) {
    getAppKit().setLispStopHook(hook);
  }

  /**
   * @brief Publish an event to the Lisp interpreter
   * @param eventID Unique identifier for the event
   * @param value Numeric value associated with the event
   *
   * Sends events from the application to the Lisp interpreter,
   * enabling bidirectional communication and script-driven
   * responses to system events.
   */
  void publishLispEvent(const String& eventID, int32_t value) {
    getAppKit().publishLispEvent(eventID, value);
  }

  /**
   * @brief Register GPIO pins as digital outputs for Lisp access
   * @param first First GPIO pin number
   * @param pins Additional GPIO pin numbers
   *
   * Makes specified GPIO pins available to the Lisp interpreter as
   * digital output pins, enabling script-based hardware control.
   */
  template <typename... Pins>
  void registerLispDigitalOutput(uint8_t first, Pins... pins) {
    uniot::PrimitiveExpeditor::getRegisterManager().setDigitalOutput(first, pins...);
  }

  /**
   * @brief Register GPIO pins as digital inputs for Lisp access
   * @param first First GPIO pin number
   * @param pins Additional GPIO pin numbers
   *
   * Makes specified GPIO pins available to the Lisp interpreter as
   * digital input pins, enabling script-based sensor reading.
   *
   * Register inputs before begin(). Registering sets the pins to plain INPUT, which clears
   * any pull; begin() gives a button on one of these pins its pull back.
   */
  template <typename... Pins>
  void registerLispDigitalInput(uint8_t first, Pins... pins) {
    uniot::PrimitiveExpeditor::getRegisterManager().setDigitalInput(first, pins...);
  }

  /**
   * @brief Register GPIO pins as analog inputs for Lisp access
   * @param first First GPIO pin number
   * @param pins Additional GPIO pin numbers
   *
   * Makes specified GPIO pins available to the Lisp interpreter as
   * analog input pins, enabling script-based analog sensor reading.
   *
   * Register inputs before begin(). Registering sets the pins to plain INPUT, which clears
   * any pull; begin() gives a button on one of these pins its pull back.
   */
  template <typename... Pins>
  void registerLispAnalogInput(uint8_t first, Pins... pins) {
    uniot::PrimitiveExpeditor::getRegisterManager().setAnalogInput(first, pins...);
  }

  /**
   * @brief Register GPIO pins as analog outputs for Lisp access
   * @param first First GPIO pin number
   * @param pins Additional GPIO pin numbers
   *
   * Makes specified GPIO pins available to the Lisp interpreter as
   * analog output pins (PWM), enabling script-based analog control.
   */
  template <typename... Pins>
  void registerLispAnalogOutput(uint8_t first, Pins... pins) {
    uniot::PrimitiveExpeditor::getRegisterManager().setAnalogOutput(first, pins...);
  }

  /**
   * @brief Register a button object with the Lisp interpreter
   * @param button Pointer to the button instance
   * @param id FOURCC identifier for the button (default: _btn)
   * @retval bool true if registration was successful, false otherwise
   *
   * Makes a button object available to the Lisp interpreter, enabling
   * script-based button event handling and state monitoring.
   */
  bool registerLispButton(uniot::Button* button, uint32_t id = FOURCC(_btn)) {
    return uniot::PrimitiveExpeditor::getRegisterManager().link(uniot::primitive::name::bclicked, button, id);
  }

  /**
   * @brief Create a button on a pin and expose it to Lisp scripts
   * @param pin GPIO pin the button is connected to
   * @param activeLevel Logic level while the button is pressed (LOW or HIGH)
   * @param id FOURCC identifier shown for the button in the device's registers (default: _btn)
   * @param callback Optional handler for CLICK and LONG_PRESS, run from the scheduler
   * @retval int The button's index, as scripts pass it to bclicked, or -1 if it could not be
   *             registered
   *
   * Everything a button needs, in one call: the Button is created and owned here, polled
   * every 100 ms, and linked into the bclicked register. A long press is 3 seconds, the same
   * as the WiFi reset button. For a button with other timing, create it yourself and use
   * registerLispButton().
   *
   * The index is the button's position in the bclicked register, so it follows registration
   * order. The WiFi reset button from configWiFiResetButton() is linked during begin(), so
   * buttons added before begin() come before it and buttons added after it follow. Adding
   * buttons after begin(), at the end of setup(), keeps the reset button at index 0.
   *
   * Add buttons in setup(). Registers are reported to the platform when MQTT connects, which
   * happens once loop() is running, so a button added later is not reported until the device
   * reconnects.
   *
   * The pin gets the internal pull its active level needs: a pull-up for LOW, a pull-down
   * for HIGH. Pins without that pull need an external resistor; see Button::applyPinMode().
   *
   * @code
   * auto door = Uniot.addLispButton(4, LOW, FOURCC(door));  // scripts use (bclicked 0)
   * @endcode
   */
  int addLispButton(uint8_t pin, uint8_t activeLevel = LOW, uint32_t id = FOURCC(_btn), uniot::Button::ButtonCallback callback = nullptr) {
    constexpr uint8_t longPressTicks = 30;
    constexpr uint32_t pollMs = 100;

    auto button = uniot::MakeShared<uniot::Button>(pin, activeLevel, longPressTicks, callback);

    auto &registers = uniot::PrimitiveExpeditor::getRegisterManager();
    if (!registers.link(uniot::primitive::name::bclicked, button.get(), id)) {
      return -1;
    }
    mButtons.push(button);

    // One task polls every button, created with the first.
    if (!mpTaskButtons) {
      mpTaskButtons = uniot::TaskScheduler::make([this](uniot::SchedulerTask &, short times) {
        mButtons.forEach([times](const uniot::SharedPointer<uniot::Button> &button) {
          button->execute(times);
        });
      });
      mScheduler.push("lisp_buttons", mpTaskButtons);
      mpTaskButtons->attach(pollMs);
    }

    return static_cast<int>(registers.getRegisterLength(uniot::primitive::name::bclicked)) - 1;
  }

  /**
   * @brief Clear pending clicks and long presses on every Lisp button
   *
   * A press that no script has read stays pending for up to 10 seconds, so a script started
   * within that window can read a press made before it existed. Call this from a start hook
   * to drop those presses:
   *
   * @code
   * Uniot.setLispStartHook([](uniot::LispStartReason) {
   *   Uniot.resetLispButtons();
   * });
   * @endcode
   *
   * Covers every button in the bclicked register, however it was added. The WiFi reset
   * button's own behaviour is unaffected: it works from its callbacks, not these flags.
   */
  void resetLispButtons() {
    _forEachLispButton([](uniot::Button *button) {
      button->resetClick();
      button->resetLongPress();
    });
  }

  /**
   * @brief Register a generic object with the Lisp interpreter
   * @param primitiveName Name of the primitive to associate with the object
   * @param link Pointer to the object to register
   * @param id FOURCC identifier for the object
   * @retval bool true if registration was successful, false otherwise
   *
   * Provides a generic mechanism for making application objects available
   * to the Lisp interpreter through named primitives.
   */
  bool registerLispObject(const String& primitiveName, uniot::RecordPtr link, uint32_t id = FOURCC(____)) {
    return uniot::PrimitiveExpeditor::getRegisterManager().link(primitiveName, link, id);
  }

  /**
   * @brief Create a repeating timer
   * @param callback Function to execute on timer expiration
   * @param intervalMs Interval between executions in milliseconds
   * @param times Number of times to execute (0 for infinite)
   * @retval TimerId Timer identifier for management operations
   *
   * Creates a timer that executes the callback function at regular intervals.
   * Similar to JavaScript's setInterval() function.
   */
  TimerId setInterval(std::function<void()> callback, uint32_t intervalMs, short times = 0) {
    if (!callback) {
      return INVALID_TIMER_ID;
    }

    auto id = _generateTimerId();
    auto task = uniot::TaskScheduler::make([this, id, callback = std::move(callback)](uniot::SchedulerTask&, short remainingTimes) {
      callback();

      if (!remainingTimes) {
        mActiveTimers.remove(id);
      }
    });

    mActiveTimers.put(id, task);
    mScheduler.push(nullptr, task);
    task->attach(intervalMs, times);
    return id;
  }

  /**
   * @brief Create a one-shot timer
   * @param callback Function to execute after timeout
   * @param timeoutMs Delay before execution in milliseconds
   * @retval TimerId Timer identifier for management operations
   *
   * Creates a timer that executes the callback function once after the
   * specified delay. Similar to JavaScript's setTimeout() function.
   */
  TimerId setTimeout(std::function<void()> callback, uint32_t timeoutMs) {
    return setInterval(std::move(callback), timeoutMs, 1);
  }

  /**
   * @brief Execute a callback on the next scheduler cycle
   * @param callback Function to execute immediately
   * @retval TimerId Timer identifier for management operations
   *
   * Schedules a callback for immediate execution on the next scheduler
   * cycle. Similar to JavaScript's setImmediate() function.
   */
  TimerId setImmediate(std::function<void()> callback) {
    return setTimeout(std::move(callback), 1);
  }

  /**
   * @brief Cancel an active timer
   * @param id Timer identifier returned by set* functions
   * @retval bool true if timer was found and canceled, false otherwise
   *
   * Stops and removes an active timer, preventing further executions.
   * Similar to JavaScript's clearInterval() and clearTimeout() functions.
   */
  bool cancelTimer(TimerId id) {
    if (id == INVALID_TIMER_ID) {
      return false;
    }

    auto task = mActiveTimers.get(id, nullptr);
    if (task) {
      task->detach();
      return mActiveTimers.remove(id);
    }
    return false;
  }

  /**
   * @brief Check if a timer is currently active
   * @param id Timer identifier to check
   * @retval bool true if timer exists and is active, false otherwise
   *
   * Determines whether a timer is currently scheduled for execution.
   */
  bool isTimerActive(TimerId id) {
    if (id == INVALID_TIMER_ID) {
      return false;
    }

    auto task = mActiveTimers.get(id, nullptr);
    return task && task->isAttached();
  }

  /**
   * @brief Get the number of active timers
   * @retval int Count of currently active timers
   *
   * Returns the current number of active timers for monitoring
   * and debugging purposes.
   */
  int getActiveTimersCount() const {
    return mActiveTimers.calcSize();
  }

  /**
   * @brief Create a named task for custom scheduling
   * @param name Optional name for the task (for debugging)
   * @param callback Function to execute when task runs
   * @retval TaskPtr Pointer to the created task for manual management
   *
   * Creates a task that can be manually scheduled and managed.
   * Provides direct access to the underlying task scheduler for
   * advanced use cases.
   */
  uniot::TaskScheduler::TaskPtr createTask(const char* name, uniot::SchedulerTask::SchedulerTaskCallback callback) {
    auto task = uniot::TaskScheduler::make(std::move(callback));
    mScheduler.push(name, task);
    return task;
  }

  /**
   * @brief Add a system event listener
   * @param callback Function to call when events occur
   * @param firstTopic First event topic to listen for
   * @param otherTopics Additional event topics to listen for
   * @retval ListenerId Listener identifier for management operations
   *
   * Registers a callback function to receive system events from multiple
   * topics. The callback receives the topic and message for event handling.
   */
  template <typename... Topics>
  ListenerId addSystemListener(std::function<void(unsigned int, int)> callback, unsigned int firstTopic, Topics... otherTopics) {
    if (!callback) {
      return INVALID_LISTENER_ID;
    }

    unsigned int topics[] = {firstTopic, static_cast<unsigned int>(otherTopics)...};
    size_t count = sizeof...(otherTopics) + 1;

    auto listener = uniot::MakeShared<uniot::CoreCallbackEventListener>(callback);
    for (size_t i = 0; i < count; ++i) {
      listener->listenToEvent(topics[i]);
    }

    if (mEventBus.registerEntity(listener.get())) {
      auto id = _generateListenerId();
      mActiveListeners.put(id, listener);
      return id;
    }

    return INVALID_LISTENER_ID;
  }

  /**
   * @brief Remove a system event listener
   * @param id Listener identifier returned by addSystemListener
   * @retval bool true if listener was found and removed, false otherwise
   *
   * Unregisters a system event listener and cleans up associated resources.
   */
  bool removeSystemListener(ListenerId id) {
    if (id == INVALID_LISTENER_ID) {
      return false;
    }

    auto listener = mActiveListeners.get(id, nullptr);
    if (listener) {
      mEventBus.unregisterEntity(listener.get());
      return mActiveListeners.remove(id);
    }
    return false;
  }

  /**
   * @brief Remove all listeners for specific topics
   * @param firstTopic First topic to remove listeners from
   * @param otherTopics Additional topics to remove listeners from
   * @retval size_t Number of listeners removed
   *
   * Removes all active listeners that are subscribed to any of the
   * specified topics. Useful for bulk cleanup operations.
   */
  template <typename... Topics>
  size_t removeSystemListeners(unsigned int firstTopic, Topics... otherTopics) {
    unsigned int topics[] = {firstTopic, static_cast<unsigned int>(otherTopics)...};
    size_t topicCount = sizeof...(otherTopics) + 1;
    size_t removedCount = 0;

    mActiveListeners.begin();
    while (!mActiveListeners.isEnd()) {
      auto listener = mActiveListeners.current().second;
      bool shouldRemove = false;

      if (listener) {
        for (size_t i = 0; i < topicCount; ++i) {
          if (listener->isListeningToEvent(topics[i])) {
            shouldRemove = true;
            break;
          }
        }
      }

      if (shouldRemove) {
        mEventBus.unregisterEntity(listener.get());
        mActiveListeners.deleteCurrent();
        removedCount++;
      } else {
        mActiveListeners.next();
      }
    }

    return removedCount;
  }

  /**
   * @brief Check if a system listener is active
   * @param id Listener identifier to check
   * @retval bool true if listener exists and is active, false otherwise
   *
   * Determines whether a system event listener is currently registered
   * and active.
   */
  bool isSystemListenerActive(ListenerId id) {
    if (id == INVALID_LISTENER_ID) {
      return false;
    }

    return mActiveListeners.exist(id);
  }

  /**
   * @brief Get the number of active event listeners
   * @retval int Count of currently active listeners
   *
   * Returns the current number of active event listeners for monitoring
   * and debugging purposes.
   */
  int getActiveListenersCount() const {
    return mActiveListeners.calcSize();
  }

  /**
   * @brief Emit a system event
   * @param topic Event topic identifier
   * @param message Event message identifier
   *
   * Broadcasts an event to all registered listeners on the specified topic.
   * Events are processed asynchronously through the event bus.
   */
  void emitSystemEvent(unsigned int topic, int message) {
    mEventBus.emitEvent(topic, message);
  }

  /**
   * @brief Add a WiFi status LED listener
   * @param callback Function to call when LED status changes
   * @retval ListenerId Listener identifier for management operations
   *
   * Convenience method for listening to WiFi status LED events.
   * The callback receives a boolean indicating the desired LED state.
   *
   * These events come from the network controller, which exists only once one of
   * the configWiFi* methods has asked for it. A device with no button and no LED
   * -- a bulb driving its own light as the indicator, say -- still gets one from
   * configWiFiResetOnReboot(). Without any of them this callback never fires; the
   * missing controller is reported by AppKit when the scheduler is populated.
   */
  ListenerId addWifiStatusLedListener(std::function<void(bool)> callback) {
    if (!callback) {
      return INVALID_LISTENER_ID;
    }

    return addSystemListener(
      [callback = std::move(callback)](unsigned int topic, int message) {
        if (topic == uniot::events::network::WIFI_STATUS_LED) {
          callback(message != 0);
        }
      },
      uniot::events::network::WIFI_STATUS_LED);
  }

  /**
   * @brief Initialize and start the Uniot platform
   * @param eventBusTaskPeriod Event bus processing interval in milliseconds
   *
   * Initializes all platform subsystems, applies configuration, and starts
   * the main event loop. This method should be called once during setup.
   */
  void begin(uint32_t eventBusTaskPeriod = 10) {
    UNIOT_LOG_SET_READY();

    auto& app = uniot::AppKit::getInstance();

    if (mpNetworkControllerConfig) {
      app.configureNetworkController(*mpNetworkControllerConfig);
      _deleteNetworkControllerConfig();
    }

    mEventBus.registerKit(app);

    auto taskHandleEventBus = uniot::TaskScheduler::make(mEventBus);
    mScheduler.push("event_bus", taskHandleEventBus);
    taskHandleEventBus->attach(eventBusTaskPeriod);

    // Registering a pin for dread or aread sets it to plain INPUT, which clears any pull. Every
    // button gets its own mode back here, so the order of those calls in setup() does not
    // matter. Before attach(), which runs the stored script.
    _forEachLispButton([](uniot::Button *button) {
      button->applyPinMode();
    });

    mScheduler.push(app);
    app.attach();
  }

  /**
   * @brief Process scheduled tasks and events
   *
   * Executes one iteration of the main scheduler loop. This method should
   * be called continuously from the main program loop to ensure proper
   * system operation.
   */
  void loop() {
    mScheduler.loop();
  }

  /**
   * @brief Get access to the application kit
   * @retval AppKit& Reference to the application kit instance
   *
   * Provides access to lower-level platform functionality for advanced
   * use cases that require direct subsystem interaction.
   */
  uniot::AppKit& getAppKit() {
    return uniot::AppKit::getInstance();
  }

  /**
   * @brief Get access to the event bus
   * @retval CoreEventBus& Reference to the event bus instance
   *
   * Provides direct access to the event bus for advanced event handling
   * and custom event types.
   */
  uniot::CoreEventBus& getEventBus() {
    return mEventBus;
  }

  /**
   * @brief Get access to the task scheduler
   * @retval TaskScheduler& Reference to the scheduler instance
   *
   * Provides direct access to the task scheduler for advanced scheduling
   * requirements and custom task management.
   */
  uniot::TaskScheduler& getScheduler() {
    return mScheduler;
  }

 private:
  /**
   * @brief Call fn for every button in the bclicked register, skipping empty and dead slots
   * @param fn Callable taking a uniot::Button*
   */
  template <typename Fn>
  void _forEachLispButton(Fn fn) {
    auto &registers = uniot::PrimitiveExpeditor::getRegisterManager();
    auto count = registers.getRegisterLength(uniot::primitive::name::bclicked);
    for (size_t i = 0; i < count; i++) {
      auto button = registers.getObject<uniot::Button>(uniot::primitive::name::bclicked, i);
      if (button) {
        fn(button);
      }
    }
  }

  /**
   * @brief Create network controller configuration if not exists
   *
   * Lazy initialization of network controller configuration with default
   * values. Called by configuration methods to ensure the config exists.
   *
   * Reboot-reset starts disabled. It clears the stored credentials, and a device on an
   * unreliable supply can power-cycle its way through the count without anyone touching
   * it, so it is opt-in: configWiFiResetOnReboot() turns it on, and that method's own
   * default argument supplies the count.
   */
  void _createNetworkControllerConfig() {
    if (!mpNetworkControllerConfig) {
      mpNetworkControllerConfig = uniot::MakeUnique<uniot::AppKit::NetworkControllerConfig>();
      mpNetworkControllerConfig->pinBtn = UINT8_MAX;  // Not used by default
      mpNetworkControllerConfig->activeLevelBtn = LOW;
      mpNetworkControllerConfig->pinLed = UINT8_MAX;  // Not used by default
      mpNetworkControllerConfig->activeLevelLed = HIGH;
      mpNetworkControllerConfig->maxRebootCount = REBOOT_RESET_DISABLED;
      mpNetworkControllerConfig->rebootWindowMs = UNIOT_WIFI_REBOOT_WINDOW_MS;
      mpNetworkControllerConfig->registerLispBtn = true;
    }
  }

  /**
   * @brief Clean up network controller configuration
   *
   * Releases memory used by network controller configuration after
   * it has been applied during initialization.
   */
  void _deleteNetworkControllerConfig() {
    if (mpNetworkControllerConfig) {
      mpNetworkControllerConfig.reset(nullptr);
    }
  }

  /**
   * @brief Generate unique timer identifier
   * @retval TimerId Unique identifier for a new timer
   *
   * Thread-safe generation of unique timer identifiers using a
   * simple incrementing counter.
   */
  TimerId _generateTimerId() {
    static TimerId nextId = 1;
    return nextId++;
  }

  /**
   * @brief Generate unique listener identifier
   * @retval ListenerId Unique identifier for a new listener
   *
   * Thread-safe generation of unique listener identifiers using a
   * simple incrementing counter.
   */
  ListenerId _generateListenerId() {
    static ListenerId nextId = 1;
    return nextId++;
  }

  uniot::TaskScheduler mScheduler;                                                                  ///< Main task scheduler instance
  uniot::CoreEventBus mEventBus;                                                                    ///< Event bus for system communication
  uniot::Map<TimerId, uniot::TaskScheduler::TaskPtr> mActiveTimers;                                 ///< Active timer tracking
  uniot::Map<ListenerId, uniot::SharedPointer<uniot::CoreCallbackEventListener>> mActiveListeners;  ///< Active listener tracking
  uniot::UniquePointer<uniot::AppKit::NetworkControllerConfig> mpNetworkControllerConfig;           ///< Network configuration (temporary)
  ClearQueue<uniot::SharedPointer<uniot::Button>> mButtons;                                         ///< Buttons created by addLispButton(), owned here
  uniot::TaskScheduler::TaskPtr mpTaskButtons;                                                      ///< Polls mButtons; created with the first button
};

/**
 * @brief Global Uniot platform instance
 *
 * Global instance providing immediate access to Uniot platform functionality.
 * This singleton-style instance eliminates the need for complex initialization
 * and provides a familiar API for Arduino developers.
 */
extern UniotCore Uniot;

/** @} */
