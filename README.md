# Uniot Core Firmware

<div align="center">

[![Version](https://img.shields.io/github/v/tag/uniot-io/uniot-core?label=version&sort=semver&color=blue)](https://github.com/uniot-io/uniot-core/tags)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Platform](https://img.shields.io/badge/platform-ESP8266%20%7C%20ESP32-lightgrey.svg)](https://platformio.org/)
[![Framework](https://img.shields.io/badge/framework-Arduino-00979D.svg)](https://www.arduino.cc/)

**A comprehensive IoT platform framework for embedded devices**

[Documentation](https://docs.uniot.io) • [Examples](#examples) • [API Reference](#api-reference) • [Contributing](#contributing)

</div>

---

## Table of Contents

- [Introduction](#introduction)
- [Key Features](#key-features)
- [Compatibility](#compatibility)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [Core Components](#core-components)
- [API Reference](#api-reference)
- [Examples](#examples)
- [Configuration](#configuration)
- [Testing](#testing)
- [Documentation](#documentation)
- [Best Practices](#best-practices)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [Community](#community)
- [License](#license)
- [Acknowledgments](#acknowledgments)

---

## Introduction

Uniot Core is a lightweight, open-source framework for building IoT devices on ESP8266 and ESP32 microcontrollers. It handles the heavy lifting of task scheduling, network management, and device communication, letting you focus on what makes your device unique. With an embedded Lisp interpreter for runtime scripting and a developer-friendly API, Uniot Core gives you both flexibility and control.

From home automation to custom devices and prototypes, Uniot Core simplifies the development process while providing the power and reliability needed for production deployments.

## Key Features

### 🚀 **Core Capabilities**

- **Non-blocking Task Scheduler**: Execute periodic and one-shot tasks efficiently without blocking
- **Event-Driven Architecture**: Decoupled communication between components via publish-subscribe pattern
- **Embedded Lisp Interpreter**: Dynamic scripting and runtime reconfiguration capabilities
- **Automatic WiFi Management**: Network connectivity with automatic reconnection and captive portal
- **MQTT Integration**: Full-featured MQTT client for cloud connectivity
- **Hardware Abstraction**: Unified GPIO management and peripheral control

### 🔒 **Security**

- **COSE Message Signing**: CBOR Object Signing and Encryption (COSE) with Ed25519, used to authenticate the device to the MQTT broker
- **Script Verification** *(planned)*: Cryptographic signature verification for remotely delivered scripts
- **Credential Storage**: WiFi and user credentials persisted on-device
- **Fault-Isolated Scripting**: Lisp scripts run on a dedicated fixed-size heap; script errors are contained and the interpreter is torn down without rebooting the device

### 💾 **Storage & Persistence**

- **CBOR-based Storage**: Efficient binary serialization for configuration and data
- **Crash Dump Capture**: Automatic crash dump saved to flash for post-mortem debugging (ESP8266)
- **LittleFS Support**: Modern filesystem for reliable flash storage
- **WiFi Credentials Storage**: Persistent credential management

### 🕐 **Time Management**

- **NTP Synchronization**: Automatic time synchronization
- **Persistent Date/Time**: Maintain time across reboots
- **Event-based Time Tracking**: Time-aware event processing

### 🔧 **Developer Experience**

- **Web-Familiar API**: Timer functions (`setTimeout`, `setInterval`, `setImmediate`) inspired by JavaScript
- **Comprehensive Logging**: Multi-level logging system for debugging
- **Doxygen Documentation**: Complete API documentation with examples
- **PlatformIO Integration**: Modern build system with dependency management

## Compatibility

Currently, Uniot Core is optimized for **ESP8266** and **ESP32** microcontrollers, two of the most popular platforms for IoT development.

### Supported Boards

- **ESP8266**: ESP-12E (tested), ESP-12F, NodeMCU, Wemos D1 Mini, and other ESP8266-based boards
- **ESP32**: ESP32 DevKit (tested), ESP32-C3 (tested), and other ESP32-family boards — ESP32-S2/S3 should work but are not routinely tested

Build environments for the tested boards are maintained in `platformio.ini` (`ESP12E`, `ESP32`, `ESP32C3`).

### Why Arduino & C++?

The decision to base Uniot Core on the Arduino framework and implement it in C++ is rooted in a commitment to:

- **Accessibility**: Arduino's user-friendly nature makes IoT development approachable
- **Performance**: C++17 provides efficiency and modern language features
- **Ecosystem**: Vast library ecosystem and thriving developer community
- **Portability**: Easy adaptation to new hardware platforms

## Installation

### Prerequisites

- [PlatformIO](https://platformio.org/) installed
- ESP8266 or ESP32 development board
- USB cable for programming

### Using PlatformIO

1. **Install PlatformIO**:

   ```bash
   pip install platformio
   ```

2. **Create a new project**:

   ```bash
   pio project init --board esp32doit-devkit-v1
   ```

3. **Configure `platformio.ini`**:

   ```ini
   [env:esp32]
   platform = espressif32        ; Use espressif8266 for ESP8266 boards
   framework = arduino
   board = esp32doit-devkit-v1
   monitor_speed = 115200

   lib_deps =
       uniot-io/uniot-core@^0.8.1

   build_unflags =
       -std=gnu++11

   build_flags =
       -std=gnu++17
       -D UNIOT_CREATOR_ID=\"YOUR_CREATOR_ID\"
       -D UNIOT_LOG_ENABLED=1
       -D UNIOT_USE_LITTLEFS=1
       -D UNIOT_LOG_LEVEL=UNIOT_LOG_LEVEL_INFO
       -D UNIOT_LISP_HEAP=10000
       -D MQTT_MAX_PACKET_SIZE=2048
   ```

   **Platform and Framework**: Ensure that the platform and framework settings match your microcontroller (e.g., `espressif8266` for ESP8266 or `espressif32` for ESP32).

   **Build Flags**:
   - `-std=gnu++17`: Required C++17 standard
   - `UNIOT_CREATOR_ID`: Device creator identifier (**required** — the build fails without it)
   - `UNIOT_LOG_ENABLED`: Enable/disable logging (1 or 0)
   - `UNIOT_USE_LITTLEFS`: Use LittleFS filesystem (1 or 0)
   - `UNIOT_LOG_LEVEL`: Logging verbosity (see [Configuration](#configuration) section)
   - `UNIOT_LISP_HEAP`: Heap size for Lisp interpreter in bytes
   - `MQTT_MAX_PACKET_SIZE`: Maximum MQTT packet size in bytes

4. **Build and upload**:

   ```bash
   pio run --target upload
   ```

## Quick Start

Here's a minimal example to get you started with Uniot Core:

```cpp
#include <Uniot.h>

void setup() {
  Serial.begin(115200);

  // Configure WiFi credentials.
  // Omit this line to let the device open a captive portal instead,
  // where the end user enters the credentials.
  Uniot.configWiFiCredentials("YourSSID", "YourPassword");

  // Configure WiFi status LED
  Uniot.configWiFiStatusLed(LED_BUILTIN);

  // Configure reset button
  Uniot.configWiFiResetButton(0, LOW);

  // Expose GPIO 12 to UniotLisp scripts as digital output 0
  Uniot.registerLispDigitalOutput(12);

  // Create a periodic task
  Uniot.setInterval([]() {
    Serial.println("Hello from Uniot!");
  }, 1000);

  // Initialize and start the platform
  Uniot.begin();
}

void loop() {
  // Execute scheduled tasks and process events
  Uniot.loop();
}
```

### What This Does

1. **Connects to WiFi** with automatic reconnection
2. **Provides visual feedback** via LED (blinking patterns for different states)
3. **Allows configuration reset** via button press
4. **Makes GPIO 12 scriptable** — remote UniotLisp scripts can drive it with `(dwrite 0 ...)`
5. **Executes periodic task** printing a message every second
6. **Manages everything automatically** through the event loop

> **Note**: Call at least one `configWiFi*` method before `Uniot.begin()` — that is what creates the network controller. Without it there is no status LED, reset button, or WiFi status events.

## Core Components

### 1. Task Scheduler

The task scheduler provides non-blocking execution of periodic and one-shot tasks:

```cpp
// Create a one-shot timer (like JavaScript's setTimeout)
Uniot.setTimeout([]() {
  Serial.println("This runs once after 5 seconds");
}, 5000);

// Create a repeating timer (like JavaScript's setInterval)
auto timerId = Uniot.setInterval([]() {
  Serial.println("This repeats every 2 seconds");
}, 2000);

// Cancel a timer
Uniot.cancelTimer(timerId);

// Execute on the next scheduler pass (~1 ms)
Uniot.setImmediate([]() {
  Serial.println("This runs almost immediately");
});

// Create a custom task with more control
auto task = Uniot.createTask("my_task", [](uniot::SchedulerTask& self, short remaining) {
  // Task implementation
  Serial.println("Custom task executed");
});
task->attach(1000); // Attach with 1 second period
```

### 2. Event System

The event bus enables decoupled communication between components:

```cpp
// Add an event listener
auto listenerId = Uniot.addSystemListener(
  [](unsigned int topic, int message) {
    Serial.printf("Event received: topic=%u, msg=%d\n", topic, message);
  },
  FOURCC(test), // First topic
  FOURCC(data)  // Additional topics
);

// Emit an event
Uniot.emitSystemEvent(FOURCC(test), 42);

// Remove listener when done
Uniot.removeSystemListener(listenerId);

// WiFi status LED listener (convenience method)
Uniot.addWifiStatusLedListener([](bool state) {
  digitalWrite(MY_LED, state ? HIGH : LOW);
});
```

### 3. WiFi Management

Uniot Core provides two ways to supply WiFi credentials to the device:

**1. Hardcoded Credentials** (programmatic setup):

Set the network credentials and Uniot account ID directly in code. Useful
for development boards or fleets where credentials are known at build time.

```cpp
Uniot.configWiFiCredentials("MyNetwork", "password123");
Uniot.configUser("your_uniot_account_id");
```

**2. Captive Portal** (user-friendly setup):

If no valid WiFi credentials are stored, the device automatically enters
Access Point mode (network name `UNIOT-XXXXXX`, where the suffix is the
device's chip ID in uppercase hex) with a captive portal where the end user can:

- Select or enter WiFi network credentials
- Enter their Uniot account ID

Credentials from both methods land in the same persistent storage. Be aware,
however, that `configWiFiCredentials()` stores its values every time it runs —
if the call stays in `setup()`, it overwrites whatever the user entered through
the captive portal on every reboot. Remove (or guard) the call once the device
is meant to be configured by its end user.

**Optional WiFi Interaction Helpers**

The methods below configure how the user interacts with the WiFi subsystem
(visual feedback, manual reset, recovery). They apply regardless of which
credential method is used and can be combined freely.

```cpp
// Status LED with custom pin and active level.
// Reflects connection state via blink patterns (see "LED Status Indicators" below).
Uniot.configWiFiStatusLed(LED_BUILTIN, HIGH);

// Reset button configuration.
// Creates a uniot::Button on the given pin and wires it to the NetworkController
// (hold ~3 s = manual reconnect; 4+ rapid clicks followed by a ~3 s hold =
// clear WiFi config, see "Resetting WiFi Configuration" below).
// The third parameter (default: true) also exposes the button to UniotLisp under
// the `bclicked` primitive. The reset button is linked during Uniot.begin(), so
// its bclicked index = number of buttons you registered via registerLispButton()
// before begin(). With no other buttons registered, the index is 0, used as
// (bclicked 0) in scripts.
Uniot.configWiFiResetButton(0, LOW, true); // pin, active level, expose to UniotLisp

// Automatic reset on repeated reboots (recovery mechanism).
// Useful when the device is physically inaccessible — power-cycling it N times
// in a row clears WiFi configuration and re-opens the captive portal.
// The reboot counter survives a window after each power-up, so every
// power-cycle in the sequence must happen within that window of the previous one.
Uniot.configWiFiResetOnReboot(5, 10000); // reset after 5 consecutive reboots, each within 10 s of power-up
```

**LED Status Indicators:**

The LED level toggles on each period, so a full on/off blink cycle takes twice the period:

- **Fast blink** (200ms toggle, ≈2.5 blinks/sec): Error/alarm state
- **Medium blink** (500ms toggle, ≈1 blink/sec): Connecting/busy
- **Slow blink** (1000ms toggle, ≈0.5 blinks/sec): Waiting/Access Point mode
- **Off**: Connected/idle

**Resetting WiFi Configuration:**

To clear current WiFi settings and switch to Access Point mode:

1. **Quick-press** the reset button **4 or more times**
2. Then **hold** it for **~3 seconds**
3. The device will clear WiFi configuration and start the captive portal

The whole sequence must be completed within **~5 seconds** of the first press —
after that the click counter resets. A ~3-second hold with fewer than 4
preceding clicks triggers a manual reconnect instead.

### 4. UniotLisp Scripting

Uniot Core includes an embedded UniotLisp interpreter for dynamic runtime scripting. Scripts can be sent via MQTT and executed on the device without reflashing firmware.

**Register Hardware for UniotLisp Access:**

Each `registerLisp*` call exposes the listed GPIO pins to UniotLisp through a
matching primitive: `dwrite` (digital write), `dread` (digital read),
`awrite` (analog/PWM write), `aread` (analog read), and `bclicked` for buttons.

Pins are not addressed by their raw GPIO number from Lisp. Instead, the
register subsystem assigns each pin a 0-based **logical index** in the order
it was registered for that primitive. Scripts then use that index — for
example, `(dwrite 0 #t)` toggles whichever pin was registered first as a
digital output. This indirection lets the same script run on different
boards without knowing the underlying pin map.

Register all pins for a primitive in a single call: calling the same
`registerLisp*` method again **replaces** the previous pin set rather than
appending to it.

```cpp
// Register GPIO pins for Lisp access.
// Indices follow registration order, per primitive.
Uniot.registerLispDigitalOutput(12, 13, 14);          // dwrite: 0=GPIO12, 1=GPIO13, 2=GPIO14
Uniot.registerLispDigitalInput(0, 4);                 // dread:  0=GPIO0, 1=GPIO4
Uniot.registerLispAnalogInput(A0);                    // aread:  0=A0
Uniot.registerLispAnalogOutput(12, 13, 14);           // awrite: 0=GPIO12, 1=GPIO13, 2=GPIO14

// Register a button object.
// The button must be polled by the scheduler to detect presses,
// so wrap it in a task and attach it (the WiFi reset button is
// wired up the same way internally by NetworkController).
auto button = new uniot::Button(0, LOW, 30); // pin, active level, long-press threshold
                                             // in poll ticks (30 ticks x 100 ms = 3 s)
auto buttonTask = uniot::TaskScheduler::make(*button);
Uniot.getScheduler().push("user_btn", buttonTask);
buttonTask->attach(100); // Poll every 100 ms
Uniot.registerLispButton(button);
```

**What Scripts Look Like:**

Once hardware is registered, scripts delivered over MQTT can drive it. A script
is built around tasks — `(task times period 'expression)` evaluates `expression`
`times` times (`0` = forever) every `period` milliseconds. For example, with a
digital output and a button registered, this script turns the output on when
the button is clicked, checking every 100 ms:

```lisp
(task 0 100 '
 (list
  (if
   (bclicked 0)
   (list
    (dwrite 0 #t)))))
```

See the [UniotLisp language description](https://docs.uniot.io/advanced/uniot-lisp/language-description) for the full syntax and built-in functions.

**Event Communication:**

```cpp
// Publish events from C++ to Lisp scripts
Uniot.publishLispEvent("sensor_reading", 42);

// Intercept incoming events (arriving via MQTT) before local scripts see them
Uniot.setLispEventInterceptor([](const uniot::LispEvent& event) {
  Serial.printf("Lisp event: %s = %d from %s\n",
    event.eventID.c_str(),
    event.value,
    event.sender.id.c_str());
  return true; // Return false to reject the event
});
```

**Creating Custom Primitives:**

Custom primitives extend the Lisp interpreter with your own functions. A primitive is a C++ function that can be called from Lisp scripts.

```cpp
using namespace uniot;

Object my_add(Root root, VarObject env, VarObject list) {
  // Describe the primitive: name, return type, number of args, arg types
  auto expeditor = PrimitiveExpeditor::describe("my_add", Lisp::Int, 2, Lisp::Int, Lisp::Int)
                     .init(root, env, list);

  // Validate arguments match the description
  expeditor.assertDescribedArgs();

  // Get arguments
  int arg1 = expeditor.getArgInt(0);
  int arg2 = expeditor.getArgInt(1);

  // Perform your custom logic
  int result = arg1 + arg2;

  // Return result
  return expeditor.makeInt(result);
}

// Register the primitive
Uniot.addLispPrimitive(my_add);
```

**Argument Types:**

- `Lisp::Int` - Integer values
- `Lisp::Bool` - Boolean values (`#t` / `()`)
- `Lisp::BoolInt` - Accepts either a boolean or an integer
- `Lisp::Symbol` - Symbols
- `Lisp::Cell` - An unevaluated list (e.g., a quoted expression)
- `Lisp::Any` - Any type

Arguments are read with `getArgInt(i)`, `getArgBool(i)`, or `getArgSymbol(i)`.

**Return Types:**

- `expeditor.makeInt(value)` - Return integer
- `expeditor.makeBool(value)` - Return boolean
- `expeditor.makeSymbol(value)` - Return symbol

For more complex primitives that interact with hardware or access device state, you can link a C++ object into the register. The object must inherit from `uniot::ObjectRegisterRecord`, and the name must match the name in the primitive's `describe()` call — the primitive then retrieves the object through `expeditor.getAssignedRegister()`:

```cpp
// Link an object so the "my-object" primitive can access it
Uniot.registerLispObject("my-object", myRecordPointer, FOURCC(myid));
```

### 5. Storage Management

Uniot Core uses CBOR (Concise Binary Object Representation) for efficient data serialization and persistent storage:

```cpp
// WiFi credentials and user ID are automatically stored
Uniot.configWiFiCredentials("SSID", "Password");
Uniot.configUser("your_account_id");

// Custom CBOR storage for your application data
uniot::CBORStorage storage("mydata.cbor");

// Store data (integers, strings, and byte arrays are supported)
storage.object()
  .put("temperature", 25)
  .put("humidity", 60)
  .put("location", "Living Room")
  .put("timestamp", static_cast<int64_t>(uniot::Date::now()));
storage.store();

// Restore data
if (storage.restore()) {
  long temp = storage.object().getInt("temperature");
  long humidity = storage.object().getInt("humidity");
  String location = storage.object().getString("location");
}
```

> **Note**: `CBORObject` stores integers (up to 64-bit), strings, and byte
> arrays — there is no floating-point support. Scale fractional values to
> integers (e.g., store 25.5 °C as 255 tenths of a degree).

### 6. Time Management

NTP synchronization and time persistence:

```cpp
// Enable periodic date saving (survives reboots)
Uniot.enablePeriodicDateSave(300); // Save every 5 minutes

// In your code, access time functions
Serial.println(uniot::Date::getFormattedTime()); // "YYYY-MM-DD HH:MM:SS"

time_t timestamp = uniot::Date::now(); // Unix timestamp in seconds
```

## API Reference

### UniotCore Class

The `Uniot` global instance provides the main API:

#### Configuration Methods

| Method                                                               | Description                    |
| -------------------------------------------------------------------- | ------------------------------ |
| `configWiFiCredentials(ssid, password = "")`                         | Set WiFi network credentials   |
| `configWiFiStatusLed(pin, activeLevel = HIGH)`                       | Configure status LED           |
| `configWiFiResetButton(pin, activeLevel = LOW, registerLisp = true)` | Configure reset button         |
| `configWiFiResetOnReboot(maxReboots, windowMs = 10000)`              | Auto-reset on repeated reboots |
| `configUser(userId)`                                                 | Set user identifier            |
| `enablePeriodicDateSave(periodSeconds = 300)`                        | Enable time persistence        |

#### Timer Methods

| Method                             | Description              | Returns   |
| ---------------------------------- | ------------------------ | --------- |
| `setTimeout(callback, ms)`         | Execute once after delay | `TimerId` |
| `setInterval(callback, ms, times)` | Execute repeatedly       | `TimerId` |
| `setImmediate(callback)`           | Execute on next cycle    | `TimerId` |
| `cancelTimer(id)`                  | Cancel a timer           | `bool`    |
| `isTimerActive(id)`                | Check if timer is active | `bool`    |
| `getActiveTimersCount()`           | Get active timer count   | `int`     |

#### Event Methods

| Method                                   | Description                     | Returns      |
| ---------------------------------------- | ------------------------------- | ------------ |
| `addSystemListener(callback, topics...)` | Add event listener              | `ListenerId` |
| `removeSystemListener(id)`               | Remove listener by ID           | `bool`       |
| `removeSystemListeners(topics...)`       | Remove all listeners for topics | `size_t`     |
| `isSystemListenerActive(id)`             | Check if listener is active     | `bool`       |
| `getActiveListenersCount()`              | Get active listener count       | `int`        |
| `emitSystemEvent(topic, message)`        | Emit an event                   | `void`       |
| `addWifiStatusLedListener(callback)`     | Add WiFi LED listener           | `ListenerId` |

#### Lisp Integration Methods

| Method                                 | Description                |
| -------------------------------------- | -------------------------- |
| `addLispPrimitive(primitive)`          | Add custom Lisp primitive  |
| `setLispEventInterceptor(interceptor)` | Set Lisp event interceptor |
| `publishLispEvent(eventID, value)`     | Publish event to Lisp      |
| `registerLispDigitalOutput(pins...)`   | Register GPIO outputs      |
| `registerLispDigitalInput(pins...)`    | Register GPIO inputs       |
| `registerLispAnalogOutput(pins...)`    | Register PWM outputs       |
| `registerLispAnalogInput(pins...)`     | Register analog inputs     |
| `registerLispButton(button, id = ...)` | Register button object     |
| `registerLispObject(name, ptr, id)`    | Register generic object    |

#### System Methods

| Method                       | Description                   | Returns      |
| ---------------------------- | ----------------------------- | ------------ |
| `begin(eventBusPeriod)`      | Initialize and start platform | `void`       |
| `loop()`                     | Process tasks and events      | `void`       |
| `createTask(name, callback)` | Create custom task            | `TaskPtr`    |
| `getAppKit()`                | Access AppKit instance        | `AppKit&`        |
| `getEventBus()`              | Access event bus instance     | `CoreEventBus&`  |
| `getScheduler()`             | Access scheduler instance     | `TaskScheduler&` |

## Examples

The repository includes several working examples demonstrating different features of Uniot Core:

### 1. WittyCloud

RGB LED controller with light sensor for WittyCloud development board.

**Location**: `examples/WittyCloud/`

**Features**:

- RGB LED control (digital and PWM)
- LDR (Light Dependent Resistor) reading
- Button input handling
- All GPIO registered for Lisp scripting
- Periodic status logging (free heap, current time)

**Hardware**: WittyCloud ESP8266 development board

**Key Code**:

```cpp
// Register all I/O for Lisp access
Uniot.registerLispDigitalOutput(PIN_RED, PIN_GREEN, PIN_BLUE);
Uniot.registerLispDigitalInput(PIN_BUTTON);
Uniot.registerLispAnalogOutput(PIN_RED, PIN_GREEN, PIN_BLUE);
Uniot.registerLispAnalogInput(PIN_LDR);
```

### 2. My9231Lamp

Smart RGB+WW+CW lamp controller with custom Lisp primitives.

**Location**: `examples/My9231Lamp/`

**Features**:

- MY9231 LED driver control (5-channel: RGB + Warm White + Cool White)
- Custom Lisp primitive `lamp_update` for remote control via MQTT
- WiFi status indication using lamp colors
- Compatible with Sonoff B1 and similar smart bulbs

**Hardware**: ESP8266-based smart bulb (Sonoff B1)

**Key Code**:

```cpp
// Custom primitive for Lisp-based control
Object lamp_update(Root root, VarObject env, VarObject list) {
  auto expeditor = PrimitiveExpeditor::describe("lamp_update", Lisp::Bool, 5,
    Lisp::Int, Lisp::Int, Lisp::Int, Lisp::Int, Lisp::Int)
    .init(root, env, list);
  // ... control lamp via Lisp scripts
}
```

### 3. S20Socket

Smart socket/relay controller with Lisp scriptable GPIO.

**Location**: `examples/S20Socket/`

**Features**:

- Relay control via GPIO
- Status LED indication
- GPIO pins registered for Lisp access (scriptable on/off control)
- WiFi configuration with reset button

**Hardware**: Sonoff S20 Smart Socket or compatible ESP8266 relay board

**Use Case**: Control appliances remotely, schedule operations, integrate with home automation

## Configuration

### Build Flags

Configure Uniot Core behavior through build flags in `platformio.ini`:

```ini
build_flags =
    -std=gnu++17
    -D UNIOT_CREATOR_ID=\"UNIOT\"      # Device creator identifier (required)
    -D UNIOT_LOG_ENABLED=1              # Enable logging
    -D UNIOT_USE_LITTLEFS=1             # Use LittleFS filesystem
    -D UNIOT_LOG_LEVEL=UNIOT_LOG_LEVEL_INFO  # Log level
    -D UNIOT_LISP_HEAP=10000           # Lisp interpreter heap size
    -D MQTT_MAX_PACKET_SIZE=2048       # MQTT packet size
```

### Log Levels

```cpp
UNIOT_LOG_LEVEL_ERROR    // Errors only
UNIOT_LOG_LEVEL_WARN     // Warnings and errors
UNIOT_LOG_LEVEL_INFO     // Info, warnings, and errors
UNIOT_LOG_LEVEL_DEBUG    // All messages including debug (default when the flag is not set)
UNIOT_LOG_LEVEL_TRACE    // All messages including trace
```

To disable logging entirely, set `UNIOT_LOG_ENABLED=0` — there is no "none" level.

### Logging

Uniot Core includes a comprehensive logging system:

```cpp
UNIOT_LOG_ERROR("Error occurred: %d", errorCode);
UNIOT_LOG_WARN("Warning: %s", message);
UNIOT_LOG_INFO("Device connected: %s", deviceId);
UNIOT_LOG_DEBUG("Debug value: %d", value);
UNIOT_LOG_TRACE("Function called: %s", __func__);

// Conditional logging (every level has an _IF variant)
UNIOT_LOG_ERROR_IF(condition, "Error if condition is true");
```

### Dependencies

Uniot Core automatically manages these dependencies:

- [uniot-cbor](https://github.com/uniot-io/uniot-cbor) - CBOR serialization
- [uniot-lisp](https://github.com/uniot-io/uniot-lisp) - Lisp interpreter
- [uniot-pubsubclient](https://github.com/uniot-io/uniot-pubsubclient) - MQTT client
- [uniot-crypto](https://github.com/uniot-io/uniot-crypto) - Cryptography support
- [uniot-esp-async-web-server](https://github.com/uniot-io/uniot-esp-async-web-server) - Async web server

## Testing

The repository ships an on-device test suite built on the [Unity](https://github.com/ThrowTheSwitch/Unity) framework, covering the CBOR layer, Lisp integration and primitives, registers, and utility types. Tests run on real hardware:

```bash
# Build, upload, and run the tests on a connected board
pio test

# Target a specific environment
pio test -e ESP32
```

## Documentation

- **Platform Documentation**: [https://docs.uniot.io](https://docs.uniot.io)
- **API Reference (Doxygen)**: [https://core.docs.uniot.io](https://core.docs.uniot.io) — generate locally with `./scripts/generate_docs.sh`
- **UniotLisp Language**: [Language Description](https://docs.uniot.io/advanced/uniot-lisp/language-description)
- **Scripting Guide**: [Scripting](https://docs.uniot.io/general-concepts/scripting)
- **Primitives**: [Primitives](https://docs.uniot.io/general-concepts/primitives)

## Best Practices

### 1. Memory Management

```cpp
// Allocate a byte buffer of a given size (contents uninitialized)
uniot::Bytes data(nullptr, 64);

// Use smart pointers for dynamic allocation
auto task = uniot::MakeShared<MyTask>();
auto buffer = uniot::MakeUnique<uint8_t[]>(1024);
```

### 2. Task Scheduling

```cpp
// Keep task execution time short
Uniot.setInterval([]() {
  // Quick operation
  sensor.read();
}, 100);

// For longer operations, use state machine pattern
Uniot.createTask("long_task", [](uniot::SchedulerTask& self, short remaining) {
  static int state = 0;
  switch(state) {
    case 0: /* Do step 1 */ state++; break;
    case 1: /* Do step 2 */ state++; break;
    case 2: /* Done */ state = 0; self.detach(); break;
  }
});
```

### 3. Event Handling

```cpp
// Remove listeners when no longer needed
void cleanup() {
  Uniot.removeSystemListener(myListenerId);
}

// Subscribe to specific topics and check the message in the callback
Uniot.addSystemListener([](unsigned int topic, int msg) {
  if (msg == uniot::events::network::Msg::SUCCESS) {
    // connected
  } else if (msg == uniot::events::network::Msg::DISCONNECTED) {
    // disconnected
  }
}, uniot::events::network::Topic::CONNECTION);
```

### 4. Error Handling

```cpp
// Always check return values
if (!Uniot.cancelTimer(timerId)) {
  UNIOT_LOG_WARN("Timer %u not found", timerId);
}

// Validate configuration
auto success = Uniot.getAppKit().setWiFiCredentials(ssid, password);
UNIOT_LOG_ERROR_IF(!success, "Invalid WiFi credentials");
```

## Troubleshooting

### WiFi Not Connecting

1. **Check credentials**: Ensure SSID and password are correct
2. **Signal strength**: Move closer to the router
3. **Reset configuration**: Quick-press the reset button 4+ times, then hold it ~3 seconds (all within ~5 seconds)
4. **Check logs**: Enable debug logging to see connection attempts

### Memory Issues

1. **Reduce Lisp heap size**: `UNIOT_LISP_HEAP=5000`
2. **Limit timer count**: Remove unused timers with `cancelTimer()`
3. **Monitor free heap**:
   ```cpp
   UNIOT_LOG_INFO("Free heap: %u", ESP.getFreeHeap());
   ```

### Upload Failures

1. **Hold boot button**: Some boards require holding BOOT during upload
2. **Check USB driver**: Ensure CH340/CP2102 driver is installed
3. **Try different baud rate**: Set `upload_speed = 115200` in platformio.ini

## Contributing

We welcome contributions! Here's how you can help:

### Reporting Issues

- Use GitHub Issues for bug reports and feature requests
- Include code examples and error logs
- Specify your hardware platform (ESP8266/ESP32)

### Pull Requests

1. **Fork** the repository
2. **Create** a feature branch (`git checkout -b feature/amazing-feature`)
3. **Commit** your changes (`git commit -m 'Add amazing feature'`)
4. **Push** to the branch (`git push origin feature/amazing-feature`)
5. **Open** a Pull Request

### Coding Standards

- Follow existing code style
- Add Doxygen comments for new APIs
- Include examples for new features
- Update documentation
- Test on both ESP8266 and ESP32

## Community

- **Website**: [https://uniot.io](https://uniot.io)
- **Forum**: [https://community.uniot.io](https://community.uniot.io)
- **GitHub**: [https://github.com/uniot-io/uniot-core](https://github.com/uniot-io/uniot-core)
- **Email**: contact@uniot.io

## License

This project is licensed under the **GNU General Public License v3.0** - see the [LICENSE](LICENSE) file for details.

### What This Means

- ✅ **Freedom to use** commercially and personally
- ✅ **Freedom to modify** and distribute modifications
- ✅ **Freedom to distribute** copies
- ⚠️ **Share-alike**: Distributed modifications must remain GPL-3.0
- ⚠️ **Source code disclosure**: Distributed modified versions must include source code

## Acknowledgments

- **Arduino Community** for the amazing framework
- **ESP8266/ESP32 Contributors** for excellent hardware support
- **All Contributors** who have helped improve Uniot Core
- **Open Source Community** for inspiration and support

---

<div align="center">

**Built with ❤️ by [Uniot Labs](https://uniot.io)**

Copyright (C) 2016-2026 Uniot Labs

</div>
