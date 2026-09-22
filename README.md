# Uniot — deprecated

> **This library was renamed to [`uniot-core`](https://registry.platformio.org/libraries/uniot-io/uniot-core).**
> `Uniot` is kept only so that existing projects keep building. It is not maintained, and no
> further versions will be released.

## What this package is

`Uniot 0.3.x` contains no functionality of its own. It depends on `uniot-core`, adds the sketch
entry points the old API provided, and emits a warning when compiled:

```
Warning: This library is deprecated. Please migrate to uniot-core.
```

It depends on `uniot-core ^0.7.2`, so it resolves to the `0.7.x` line and will never pull a
newer release.

## Migrating to uniot-core

**1. Change the dependency in `platformio.ini`:**

```ini
lib_deps =
    uniot-io/uniot-core
```

**2. Define `setup()` and `loop()` yourself.** This package defines them for you and calls your
`inject()`; `uniot-core` does not:

```cpp
// before — Uniot
#include <UniotLegacy.h>

void inject() {
  // your code
}
```

```cpp
// after — uniot-core
#include <Uniot.h>

void setup() {
  Uniot.begin();
  // your code
}

void loop() {
  Uniot.loop();
}
```

**3. Replace the globals:**

| Uniot            | uniot-core            |
| ---------------- | --------------------- |
| `MainScheduler`  | `Uniot.getScheduler()`|
| `MainEventBus`   | `Uniot.getEventBus()` |

Releases after `0.7.x` carry breaking changes — read the
[changelog](https://github.com/uniot-io/uniot-core/blob/master/CHANGELOG.md) before upgrading, and
see the [documentation](https://github.com/uniot-io/uniot-core#readme) and
[examples](https://github.com/uniot-io/uniot-core/tree/master/examples) to get started.

## License

GPL-3.0 — see [LICENSE](LICENSE).

---

The source of this package lives on the
[`pio-legacy`](https://github.com/uniot-io/uniot-core/tree/pio-legacy) branch of `uniot-core`.
