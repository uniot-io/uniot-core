# Uniot Core Firmware

<div align="center">

[![Version](https://img.shields.io/github/v/tag/uniot-io/uniot-core?label=version&sort=semver&color=blue)](https://github.com/uniot-io/uniot-core/tags)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Platform](https://img.shields.io/badge/platform-ESP8266%20%7C%20ESP32-lightgrey.svg)](https://platformio.org/)
[![Framework](https://img.shields.io/badge/framework-Arduino-00979D.svg)](https://www.arduino.cc/)

**A comprehensive IoT platform framework for embedded devices**

[Documentation](https://docs.uniot.io) • [Reference](docs/reference.md) • [Examples](#examples) • [Contributing](#contributing)

</div>

---

## Table of Contents

- [Introduction](#introduction)
- [What You Get](#what-you-get)
- [Compatibility](#compatibility)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [Scripting](#scripting)
- [Examples](#examples)
- [Configuration](#configuration)
- [Diagnostics](#diagnostics)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [Community](#community)
- [License](#license)
- [Acknowledgments](#acknowledgments)

---

## Introduction

Uniot Core is a lightweight, open-source framework for building IoT devices on ESP8266 and ESP32 microcontrollers. It handles the heavy lifting of task scheduling, network management, and device communication, letting you focus on what makes your device unique. With an embedded Lisp interpreter for runtime scripting and a developer-friendly API, Uniot Core gives you both flexibility and control.

From home automation to custom devices and prototypes, Uniot Core simplifies the development process while providing the power and reliability needed for production deployments.

## What You Get

- **Non-blocking task scheduler** with `setTimeout`, `setInterval` and `setImmediate`
- **Event bus** for decoupled publish-subscribe communication between components
- **Embedded UniotLisp interpreter**, so device behaviour can be changed over MQTT without
  reflashing. Scripts run on a fixed-size heap; a failing script is torn down without
  rebooting the device
- **WiFi management** with automatic reconnection, a captive portal for end-user setup, and
  recovery paths for a device that can no longer reach its network
- **MQTT client** that authenticates to the broker and signs what it publishes with
  COSE/Ed25519
- **CBOR storage** on LittleFS for credentials, configuration and your own data
- **NTP time** that survives reboots

See the [reference](docs/reference.md) for how each of these is used.

## Compatibility

| Family | Boards |
| --- | --- |
| ESP8266 | ESP-12E, ESP-12F, NodeMCU, Wemos D1 Mini |
| ESP32 | ESP32 DevKit, ESP32-C3, ESP32-S2, ESP32-S3 |

Ready-made build environments live in `platformio.ini` as `ESP12E`, `ESP32` and `ESP32C3`. The framework targets the Arduino core and requires C++17.

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
       uniot-io/uniot-core@^0.9.0

   build_unflags =
       -std=gnu++11

   build_flags =
       -std=gnu++17
       -D UNIOT_CREATOR_ID=\"YOUR_CREATOR_ID\"
       -D UNIOT_LOG_ENABLED=1
       -D UNIOT_USE_LITTLEFS=1
       -D UNIOT_LOG_LEVEL=UNIOT_LOG_LEVEL_INFO
       -D MQTT_MAX_PACKET_SIZE=2048
   ```

   **Platform and Framework**: Ensure that the platform and framework settings match your microcontroller (e.g., `espressif8266` for ESP8266 or `espressif32` for ESP32).

   Every flag is described under [Configuration](#configuration). `UNIOT_CREATOR_ID` is
   the only required one — the build fails without it.

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

1. **Connects to WiFi** with automatic reconnection, falling back to a captive portal
2. **Gives the user feedback and a way out** — LED blink patterns, and a button that resets the stored configuration
3. **Makes GPIO 12 scriptable** — remote UniotLisp scripts drive it with `(dwrite 0 ...)`
4. **Runs a periodic task** and processes everything from the event loop

> **Note**: Call at least one `configWiFi*` method before `Uniot.begin()` — that is what creates the network controller. Without it there is no status LED, reset button, or WiFi status events. A device with neither button nor LED can still create it with `configWiFiResetOnReboot()`.

## Scripting

Scripts are written in UniotLisp, delivered over MQTT, and run without reflashing. Your
sketch decides what they can reach:

```cpp
Uniot.registerLispDigitalOutput(12, 13, 14);  // dwrite
Uniot.registerLispDigitalInput(0, 4);         // dread
Uniot.registerLispAnalogInput(A0);            // aread
```

**Pins are not addressed by GPIO number.** Each `registerLisp*` call assigns its pins a
0-based index in registration order, per primitive, and scripts use that index — so
`(dwrite 0 #t)` drives whichever pin was registered first as a digital output. This is what
lets one script run on boards with different pin maps. Registering again **replaces** the
previous set rather than adding to it, so list every pin in a single call.

- [Language description](https://docs.uniot.io/advanced/uniot-lisp/language-description)
- [Scripting guide](https://docs.uniot.io/general-concepts/scripting) and [primitives](https://docs.uniot.io/general-concepts/primitives)
- [Custom primitives, events and registered objects](docs/reference.md#4-uniotlisp-scripting)

## Examples

Each directory is a self-contained PlatformIO project.

| Example | Hardware | Shows |
| --- | --- | --- |
| [WittyCloud](examples/WittyCloud/) | WittyCloud ESP8266 board | RGB output, light sensor, button, everything exposed to scripts |
| [My9231Lamp](examples/My9231Lamp/) | ESP8266 smart bulb with a MY9231 LED driver | A custom Lisp primitive, and a device with no button or status LED |
| [S20Socket](examples/S20Socket/) | ESP8266 smart socket or relay board | Relay control and a scriptable GPIO |
| [LispHooks](examples/LispHooks/) | Any ESP8266 board | Powering a sensor from the script lifecycle hooks |
| [Buttons](examples/Buttons/) | WittyCloud ESP8266 board | Two buttons exposed to scripts, and clearing stale presses |

## Configuration

### Build Flags

Configure Uniot Core behavior through build flags in `platformio.ini`:

```ini
build_flags =
    -std=gnu++17
    -D UNIOT_CREATOR_ID=\"UNIOT\"           # Device creator identifier (required)
    -D UNIOT_LOG_ENABLED=1                   # Enable logging
    -D UNIOT_USE_LITTLEFS=1                  # Use LittleFS filesystem
    -D UNIOT_LOG_LEVEL=UNIOT_LOG_LEVEL_INFO  # Log level
    -D MQTT_MAX_PACKET_SIZE=2048             # MQTT packet size
```

`lib/Core/Common.h` carries a default for every value below, so define one only to
override it:

| Flag | Default | Purpose |
| --- | --- | --- |
| `UNIOT_MQTT_HOST` | `"mqtt.uniot.io"` | Broker to connect to |
| `UNIOT_MQTT_PORT` | `1883` | Broker port |
| `UNIOT_WIFI_AP_PREFIX` | `"UNIOT"` | SSID prefix of the configuration portal |
| `UNIOT_WIFI_AP_PASSWORD` | `""` (open) | Password for that portal |
| `UNIOT_WIFI_NO_SLEEP` | `0` | Disable WiFi modem sleep |
| `UNIOT_WIFI_REBOOT_RESET_COUNT` | `5` | Power cycles that clear stored credentials, once `configWiFiResetOnReboot()` enables it |
| `UNIOT_WIFI_REBOOT_WINDOW_MS` | `10000` | How long a reboot still counts towards that |
| `UNIOT_LISP_HEAP` | 24576 (ESP32) / 12288 (ESP8266) | Interpreter heap, in bytes |
| `UNIOT_LISP_MAX_EVAL_STACK` | 3072 (ESP32) / 1280 (ESP8266) | Evaluation stack budget, in bytes |

The two Lisp values were measured against the full firmware rather than chosen, and the
ESP8266 numbers have little room: the old 1280 budget's predecessor ran 112 bytes from a
stack overflow. Raise `UNIOT_LISP_MAX_EVAL_STACK` only with a measurement from
`tools/LispEvalDepth` on the same board and build.

### Log Levels

```cpp
UNIOT_LOG_LEVEL_ERROR    // Errors only
UNIOT_LOG_LEVEL_WARN     // Warnings and errors
UNIOT_LOG_LEVEL_INFO     // Info, warnings, and errors
UNIOT_LOG_LEVEL_DEBUG    // All messages including debug (default when the flag is not set)
UNIOT_LOG_LEVEL_TRACE    // All messages including trace
```

To disable logging entirely, set `UNIOT_LOG_ENABLED=0` — there is no "none" level.

### Dependencies

Uniot Core automatically manages these dependencies:

- [uniot-cbor](https://github.com/uniot-io/uniot-cbor) - CBOR serialization
- [uniot-lisp](https://github.com/uniot-io/uniot-lisp) - Lisp interpreter
- [uniot-pubsubclient](https://github.com/uniot-io/uniot-pubsubclient) - MQTT client
- [uniot-crypto](https://github.com/uniot-io/uniot-crypto) - Cryptography support
- [uniot-esp-async-web-server](https://github.com/uniot-io/uniot-esp-async-web-server) - Async web server

## Diagnostics

`tools/` holds standalone PlatformIO sketches that measure interpreter behaviour on real
hardware — `LispEvalDepth` reports how much stack an evaluation consumes, and `LispGC`
exercises the collector under memory pressure. Each is its own project, built from its own
directory:

```bash
cd tools/LispEvalDepth
pio run -t upload -t monitor
```

These are measuring instruments, not a test suite: they print numbers for a human to read,
and the eval-stack and heap budgets in `lib/Core/Common.h` were set from their output.

## Documentation

- **Platform Documentation**: [https://docs.uniot.io](https://docs.uniot.io)
- **Reference**: [docs/reference.md](docs/reference.md) — core components, API tables, best practices and troubleshooting, pending a move to the docs site
- **API Reference (Doxygen)**: [https://core.docs.uniot.io](https://core.docs.uniot.io) — generate locally with `./scripts/generate_docs.sh`
- **UniotLisp Language**: [Language Description](https://docs.uniot.io/advanced/uniot-lisp/language-description)
- **Scripting Guide**: [Scripting](https://docs.uniot.io/general-concepts/scripting)
- **Primitives**: [Primitives](https://docs.uniot.io/general-concepts/primitives)

## Contributing

We welcome contributions! Here's how you can help:

### Reporting Issues

- Use GitHub Issues for bug reports and feature requests
- Include code examples and error logs
- Specify your hardware platform (ESP8266/ESP32)

### Pull Requests

Branch from `master`, keep the existing code style, add Doxygen comments for new APIs, and
say in the description which boards you tested on.



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
