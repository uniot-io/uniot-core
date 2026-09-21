# Changelog

## 0.9.0

The first release to carry a version the platform can read, and the one that moves the Lisp
interpreter to 0.4.0. **Read the breaking list before updating a device**: the language
changed under stored scripts, and analog readings changed scale on ESP32.

### Breaking

**The interpreter is uniot-lisp 0.4.0, which changed the language.** Scripts written against
0.2.x need review:

- `eq` is now `eql`. The old `eq` compared by address, so `(eq 1 1)` was false.
- Only `()` is false. `(not 0)` used to be true and is now `()`, which silently changes any
  toggle built on a pin read or an event payload.
- Arithmetic raises on overflow instead of wrapping, `/` is exact, calling with the wrong
  number of arguments raises in both directions, and duplicate parameter names are refused.

**`aread` returns 0-1023 on every chip.** It previously returned the raw ADC width: 0-4095 on
ESP32 and C3, 0-8191 on S2 and S3. Thresholds tuned against those need rescaling. `awrite`
was already normalised to the same range. Both ranges are now applied only when a script
registers the matching pins, so a sketch that never exposes analog keeps its own settings.

**Reset-on-reboot is opt-in.** It clears the stored credentials, and a device on an unreliable
supply can power-cycle its way through the count untouched, so it now requires
`configWiFiResetOnReboot()`. `UNIOT_WIFI_REBOOT_RESET_COUNT` and `UNIOT_WIFI_REBOOT_WINDOW_MS`
carry the defaults.

**An empty script stops the interpreter** instead of being ignored, reported to a stop hook as
`Cleared`. A script packet with no `code` field is now ignored rather than treated as empty.

**The ESP8266 crash storage was removed.**

**The Lisp limits moved to `Common.h` and were measured on target**, not chosen:
`UNIOT_LISP_HEAP` is 24576 on ESP32 and 12288 on ESP8266, and `UNIOT_LISP_MAX_EVAL_STACK` is
3072 and 1280. The previous ESP8266 stack value ran 112 bytes from an overflow.

### Added

- **Script lifecycle hooks.** `setLispStartHook()` and `setLispStopHook()` bracket a script's
  interpreter, with a reason for each: `Restored` or `Received` for a start, and `Completed`,
  `Replaced`, `Cleared` or `Failed` for a stop. This is where a peripheral a script depends on
  is prepared and shut down.
- **`addLispButton()`**, which creates a button, polls it and registers it for `bclicked` in
  one call, and **`resetLispButtons()`**, which drops presses no script has read yet.
- **A device status the platform can act on**: `version`, `lisp_version`, `lisp_stack` and
  `mcu`, and `reset_reason` on ESP8266 as well as ESP32.
- **`UNIOT_CORE_VERSION`**, packed by `UNIOT_SEMVER_TO_INT` the same way the interpreter packs
  its own, and deliberately not overridable by a sketch.
- **Build-time configuration** for the broker and the configuration portal: `UNIOT_MQTT_HOST`,
  `UNIOT_MQTT_PORT`, `UNIOT_WIFI_AP_PREFIX`, `UNIOT_WIFI_AP_PASSWORD`, `UNIOT_WIFI_NO_SLEEP`.
- **Portal and MQTT extensibility**: `addCustomPage()`, `addCustomRoute()` and
  `addMQTTDevice()`, so user code can extend the configuration portal and speak its own MQTT
  topics.
- **`SchedulerTask::trigger()`**, which runs a task on the scheduler's next pass and is safe to
  call from an SDK callback or another FreeRTOS task.
- **A log sink**, `uniot_log_set_sink()`, for mirroring log lines somewhere else.
- **Examples and tools**: `LispHooks` and `Buttons`, plus `tools/LispEvalDepth` and
  `tools/LispGC` for measuring interpreter limits on real hardware.
- **`docs/reference.md`**, with the API tables, the scripting model and the status packet.

### Fixed

- **Time syncs no longer write flash from a callback.** On ESP8266 the core runs
  `settimeofday_cb` from inside `yield()`, including the yields LittleFS makes while writing,
  which started a second filesystem operation inside one already in progress and tripped a
  LittleFS assertion. On ESP32 the same callback runs on the network task. Both now only
  trigger a scheduler task. A forced sync also stored the time from *before* the sync.
- **A long button press fired twice.** The counter is a `uint8_t` and kept running, so it
  wrapped after 255 ticks; a very long hold also registered as a click on release.
- **A button's pin no longer needs an external resistor on most pins.** The internal pull is
  chosen from the active level, and `begin()` re-applies it, so registering the same pin for
  `dread` can no longer strip it.
- **A failed CBOR build no longer destroys the stored file.** It was written anyway, which
  truncated the file to nothing on LittleFS and removed the key on NVS.
- **`Bytes` is safe when an allocation fails**: `c_str()` never returns null, a failed
  reservation leaves the object unchanged instead of reporting a size it does not have, and
  move construction and assignment avoid copying payloads.
- **Credentials reject a stored private key of the wrong length** rather than reading past it,
  and signing uses the public key derived at startup instead of deriving it again every time.
- **Storage reads files through `Bytes`** rather than an unchecked `malloc`.
- **Log messages live in flash**, which frees about 6 KB of RAM on ESP8266. `Logger.h` also
  includes `Common.h`, so including it alone compiles when a log level is filtered out.
- **ESP32-C3 pin assignments.** `defined(ESP32)` is also true for the C3, so GPIO 8 and 10 were
  driven onto the SPI flash bus and the chip stopped at boot with no panic output.
- **The NTP retry task is re-armed unconditionally** on a failed sync.
- **The configuration portal is addressed by IP**, so captive-portal detection works.

### Known limitations

- **Incoming messages are not verified.** `COSEMessage::verify()` exists but nothing calls it,
  so the device trusts what the broker delivers on its topics. Signing outbound messages is
  unaffected.
- **MQTT reconnection has no backoff.** While the broker is unreachable the device retries
  continuously, and each attempt signs its credentials again.
