# Uniot Core Firmware

<div align="center">

[![Version](https://img.shields.io/badge/version-0.8.1-blue.svg)](https://github.com/uniot-io/uniot-core)
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
- [Documentation](#documentation)
- [Best Practices](#best-practices)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [Community](#community)
- [License](#license)

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

- **COSE Message Signing**: Support for CBOR Object Signing and Encryption (COSE) standard
- **Script Verification**: Cryptographic signature verification for remote scripts
- **Secure Storage**: Protected credential management for WiFi and user authentication
- **Sandboxed Execution**: Isolated Lisp interpreter environment for safe script execution

### 💾 **Storage & Persistence**

- **CBOR-based Storage**: Efficient binary serialization for configuration and data
- **Crash Recovery**: Automatic crash detection and reporting
- **LittleFS Support**: Modern filesystem for reliable flash storage
- **WiFi Credentials Storage**: Secure credential management

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

- **ESP8266**: ESP-12E, ESP-12F, NodeMCU, Wemos D1 Mini, and compatible boards
- **ESP32**: ESP32 DevKit, ESP32-C3, ESP32-S2, ESP32-S3, and compatible boards

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
   - `UNIOT_CREATOR_ID`: Device creator identifier (default: "UNIOT")
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

  // Configure WiFi credentials
  Uniot.configWiFiCredentials("YourSSID", "YourPassword");

  // Configure WiFi status LED
  Uniot.configWiFiStatusLed(LED_BUILTIN);

  // Configure reset button
  Uniot.configWiFiResetButton(0, LOW);

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
4. **Executes periodic task** printing a message every second
5. **Manages everything automatically** through the event loop

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

// Execute immediately on next cycle
Uniot.setImmediate([]() {
  Serial.println("This runs immediately");
});

// Create a custom task with more control
auto task = Uniot.createTask("my_task", [](SchedulerTask& self, short remaining) {
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

Uniot Core provides two ways to connect your device to WiFi:

**1. Hardcoded Credentials** (programmatic setup):

```cpp
// Configure WiFi credentials
Uniot.configWiFiCredentials("MyNetwork", "password123");

// Configure user identification (Uniot account ID)
Uniot.configUser("your_uniot_account_id");

// Status LED with custom pin and active level
Uniot.configWiFiStatusLed(LED_BUILTIN, HIGH);

// Reset button configuration
Uniot.configWiFiResetButton(0, LOW, true); // Pin, active level, register with Lisp

// Automatic reset on repeated reboots (recovery mechanism)
Uniot.configWiFiResetOnReboot(5, 10000); // 5 reboots within 10 seconds triggers reset
```

**2. Captive Portal** (user-friendly setup):

If no valid WiFi credentials are found, the device automatically enters Access Point mode with a captive portal where users can:

- Select or enter WiFi network credentials
- Enter their Uniot account ID

**LED Status Indicators:**

- **5 blinks/sec** (200ms period): Error/alarm state
- **2 blinks/sec** (500ms period): Connecting/busy
- **1 blink/sec** (1000ms period): Waiting/Access Point mode
- **Brief flash**: Connected/idle

**Resetting WiFi Configuration:**

To clear current WiFi settings and switch to Access Point mode:

1. **Quick press** the reset button **5-8 times**
2. Then **hold** for **3-5 seconds**
3. The device will clear WiFi configuration and start the captive portal

### 4. Lisp Scripting

Uniot Core includes an embedded Lisp interpreter for dynamic runtime scripting. Scripts can be sent via MQTT and executed on the device without reflashing firmware.

**Register Hardware for Lisp Access:**

```cpp
// Register GPIO pins for Lisp access
Uniot.registerLispDigitalOutput(LED_BUILTIN, 12, 13);
Uniot.registerLispDigitalInput(0, 4);
Uniot.registerLispAnalogInput(A0);
Uniot.registerLispAnalogOutput(12, 13, 14);

// Register a button object
auto button = new uniot::Button(0, LOW);
Uniot.registerLispButton(button);
```

**Event Communication:**

```cpp
// Publish events from C++ to Lisp scripts
Uniot.publishLispEvent("sensor_reading", 42);

// Intercept events from Lisp scripts
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
- `Lisp::Bool` - Boolean values
- `Lisp::Symbol` - Symbols

**Return Types:**

- `expeditor.makeInt(value)` - Return integer
- `expeditor.makeBool(value)` - Return boolean
- `expeditor.makeSymbol(value)` - Return symbol

For more complex primitives that interact with hardware or access device state, you can use the `RegisterManager` to link objects:

```cpp
// Register a custom object that can be accessed by primitives
Uniot.registerLispObject("my-object", myObjectPointer, FOURCC(myid));
```

### 5. Storage Management

Uniot Core uses CBOR (Concise Binary Object Representation) for efficient data serialization and persistent storage:

```cpp
// WiFi credentials and user ID are automatically stored
Uniot.configWiFiCredentials("SSID", "Password");
Uniot.configUser("your_account_id");

// Custom CBOR storage for your application data
uniot::CBORStorage storage("mydata.cbor");

// Store data
storage.object()
  .put("temperature", 25.5)
  .put("humidity", 60)
  .put("location", "Living Room")
  .put("timestamp", uniot::Date::now());
storage.store();

// Restore data
if (storage.restore()) {
  double temp = storage.object().getDouble("temperature");
  int humidity = storage.object().getInt("humidity");
  String location = storage.object().getString("location");
}
```

### 6. Time Management

NTP synchronization and time persistence:

```cpp
// Enable periodic date saving (survives reboots)
Uniot.enablePeriodicDateSave(300); // Save every 5 minutes

// In your code, access time functions
Serial.println(uniot::Date::getFormattedTime());
Serial.println(uniot::Date::getFormattedDate());

uint64_t timestamp = uniot::Date::now(); // Unix timestamp in seconds
```

## API Reference

### UniotCore Class

The `Uniot` global instance provides the main API:

#### Configuration Methods

| Method                                                  | Description                    |
| ------------------------------------------------------- | ------------------------------ |
| `configWiFiCredentials(ssid, password)`                 | Set WiFi network credentials   |
| `configWiFiStatusLed(pin, activeLevel)`                 | Configure status LED           |
| `configWiFiResetButton(pin, activeLevel, registerLisp)` | Configure reset button         |
| `configWiFiResetOnReboot(maxReboots, windowMs)`         | Auto-reset on repeated reboots |
| `configUser(userId)`                                    | Set user identifier            |
| `enablePeriodicDateSave(periodSeconds)`                 | Enable time persistence        |

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
| `registerLispButton(button, id)`       | Register button object     |
| `registerLispObject(name, ptr, id)`    | Register generic object    |

#### System Methods

| Method                       | Description                   | Returns      |
| ---------------------------- | ----------------------------- | ------------ |
| `begin(eventBusPeriod)`      | Initialize and start platform | `void`       |
| `loop()`                     | Process tasks and events      | `void`       |
| `createTask(name, callback)` | Create custom task            | `TaskPtr`    |
| `getAppKit()`                | Access AppKit instance        | `AppKit&`    |
| `getEventBus()`              | Access EventBus instance      | `EventBus&`  |
| `getScheduler()`             | Access Scheduler instance     | `Scheduler&` |

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
- Periodic sensor monitoring

**Hardware**: WittyCloud ESP8266 development board

**Key Code**:

```cpp
// Register all I/O for Lisp access
Uniot.registerLispDigitalOutput(PIN_RED, PIN_GREEN, PIN_BLUE);
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
    -D UNIOT_CREATOR_ID=\"UNIOT\"      # Device creator identifier
    -D UNIOT_LOG_ENABLED=1              # Enable logging
    -D UNIOT_USE_LITTLEFS=1             # Use LittleFS filesystem
    -D UNIOT_LOG_LEVEL=UNIOT_LOG_LEVEL_DEBUG  # Log level
    -D UNIOT_LISP_HEAP=10000           # Lisp interpreter heap size
    -D MQTT_MAX_PACKET_SIZE=2048       # MQTT packet size
```

### Log Levels

```cpp
UNIOT_LOG_LEVEL_NONE     // No logging
UNIOT_LOG_LEVEL_ERROR    // Errors only
UNIOT_LOG_LEVEL_WARN     // Warnings and errors
UNIOT_LOG_LEVEL_INFO     // Info, warnings, and errors
UNIOT_LOG_LEVEL_DEBUG    // All messages including debug
UNIOT_LOG_LEVEL_TRACE    // All messages including trace
```

### Logging

Uniot Core includes a comprehensive logging system:

```cpp
UNIOT_LOG_ERROR("Error occurred: %d", errorCode);
UNIOT_LOG_WARN("Warning: %s", message);
UNIOT_LOG_INFO("Device connected: %s", deviceId);
UNIOT_LOG_DEBUG("Debug value: %d", value);
UNIOT_LOG_TRACE("Function called: %s", __func__);

// Conditional logging
UNIOT_LOG_ERROR_IF(condition, "Error if condition is true");
```

### Dependencies

Uniot Core automatically manages these dependencies:

- [uniot-cbor](https://github.com/uniot-io/uniot-cbor) - CBOR serialization
- [uniot-lisp](https://github.com/uniot-io/uniot-lisp) - Lisp interpreter
- [uniot-pubsubclient](https://github.com/uniot-io/uniot-pubsubclient) - MQTT client
- [uniot-crypto](https://github.com/uniot-io/uniot-crypto) - Cryptography support
- [uniot-esp-async-web-server](https://github.com/uniot-io/uniot-esp-async-web-server) - Async web server

## Documentation

Complete API documentation is generated with Doxygen and available at:

- **Online**: [https://core.docs.uniot.io](https://core.docs.uniot.io)
- **Local**: Generate with `./scripts/generate_docs.sh`

## Best Practices

### 1. Memory Management

```cpp
// Prefer stack allocation for small objects
uniot::Bytes data(64);

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
Uniot.createTask("long_task", [](SchedulerTask& self, short remaining) {
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

// Use specific topics instead of listening to everything
Uniot.addSystemListener(handler,
  uniot::events::network::CONNECTED,  // Specific events only
  uniot::events::network::DISCONNECTED
);
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
3. **Reset configuration**: Press reset button multiple times rapidly, then hold
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
- ⚠️ **Share-alike**: Modifications must also be GPL-3.0
- ⚠️ **Source code disclosure**: Modified versions must include source code

## Acknowledgments

- **Arduino Community** for the amazing framework
- **ESP8266/ESP32 Contributors** for excellent hardware support
- **All Contributors** who have helped improve Uniot Core
- **Open Source Community** for inspiration and support

---

<div align="center">

**Built with ❤️ by [Uniot Labs](https://uniot.io)**

Copyright (C) 2025 Uniot Labs

</div>
