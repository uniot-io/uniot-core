/*
 * Touches the public API so a build failure here means the library cannot be consumed,
 * not that this sketch is wrong. It is never flashed.
 */

#include <Uniot.h>

using namespace uniot;

void setup() {
  Uniot.configWiFiStatusLed(2);
  Uniot.configWiFiResetButton(0, LOW);
  Uniot.configWiFiResetOnReboot();

  Uniot.registerLispDigitalOutput(4);
  Uniot.registerLispDigitalInput(5);
  Uniot.registerLispAnalogInput(A0);

  Uniot.setLispStartHook([](LispStartReason) {
    Uniot.resetLispButtons();
  });
  Uniot.setLispStopHook([](LispStopReason) {});

  Uniot.setInterval([]() {
    UNIOT_LOG_INFO("uptime: %lu", millis());
  }, 5000);

  Uniot.begin();

  Uniot.addLispButton(12, LOW, FOURCC(test));
}

void loop() {
  Uniot.loop();
}
