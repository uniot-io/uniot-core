# Uniot Core Reference

Detailed reference material for Uniot Core, kept out of the README so that page stays a
landing page. This is a staging file: it is destined for [docs.uniot.io](https://docs.uniot.io),
and the API tables below duplicate what Doxygen already generates from the headers
([core.docs.uniot.io](https://core.docs.uniot.io)) — prefer the generated version when the two
disagree.

## Contents

- [Core Components](#core-components)
- [API Reference](#api-reference)
- [Logging](#logging)
- [Best Practices](#best-practices)
- [Troubleshooting](#troubleshooting)

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
// Off unless you call this -- it clears the credentials, and a device on an
// unreliable supply could otherwise power-cycle its way through the count.
// Useful when the device is physically inaccessible — power-cycling it N times
// in a row clears WiFi configuration and re-opens the captive portal.
// The reboot counter survives a window after each power-up, so every
// power-cycle in the sequence must happen within that window of the previous one.
Uniot.configWiFiResetOnReboot();         // enable, with the UNIOT_WIFI_REBOOT_* defaults
Uniot.configWiFiResetOnReboot(5, 10000); // or state the count and window explicitly
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
(task 0 100 '(if (bclicked 0) (dwrite 0 #t)))
```

True is `#t` and false is `()` — there is no `#f`. Use `progn` where one expression
is expected but several are needed, such as an `if` branch with more than one thing
to do:

```lisp
(task 0 100 '(if (bclicked 0)
              (progn
               (dwrite 0 #t)
               (dwrite 1 ()))))
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
| `configWiFiResetOnReboot(maxReboots = ..., windowMs = ...)`          | Enable auto-reset on repeated reboots (off by default) |
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

## Logging

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

1. **Reduce Lisp heap size**: lower `UNIOT_LISP_HEAP` below the default for your chip. Scripts that outgrow it will start failing to allocate, so cut it in steps rather than in one jump.
2. **Limit timer count**: Remove unused timers with `cancelTimer()`
3. **Monitor free heap**:
   ```cpp
   UNIOT_LOG_INFO("Free heap: %u", ESP.getFreeHeap());
   ```

### Upload Failures

1. **Hold boot button**: Some boards require holding BOOT during upload
2. **Check USB driver**: Ensure CH340/CP2102 driver is installed
3. **Try different baud rate**: Set `upload_speed = 115200` in platformio.ini

