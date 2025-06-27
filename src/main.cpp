#include <Uniot.h>

#include "primitives.h"

#if defined(ESP8266)
#define PIN_LDR A0
#define PIN_RED 15
#define PIN_GREEN 12
#define PIN_BLUE 13
#define PIN_LED 2
#define PIN_BUTTON 4
#define LED_PIN_LEVEL HIGH
#define BTN_PIN_LEVEL LOW
#elif defined(ESP32)
#define PIN_VIBRO 10
#define PIN_LED 8
#define PIN_BUTTON 3
#define LED_PIN_LEVEL LOW
#define BTN_PIN_LEVEL LOW
#endif

using namespace uniot;

auto taskPrintHeap = Uniot.createTask("print_heap", [](SchedulerTask& self, short t) {
  Serial.println(ESP.getFreeHeap());
});

auto taskPrintTime = Uniot.createTask("print_time", [](SchedulerTask& self, short t) {
  Serial.println(Date::getFormattedTime());
});

void setup() {
  Serial.begin(115200);
  Uniot.configWiFiResetButton(PIN_BUTTON, BTN_PIN_LEVEL);
  Uniot.configWiFiResetOnReboot(5);

#if defined(ESP8266)
  Uniot.configWiFiStatusLed(PIN_RED, LED_PIN_LEVEL);
  Uniot.registerLispDigitalOutput(PIN_RED, PIN_GREEN, PIN_BLUE);
  Uniot.registerLispDigitalInput(0, PIN_BUTTON);
  Uniot.registerLispAnalogOutput(PIN_RED, PIN_GREEN, PIN_BLUE);
  Uniot.registerLispAnalogInput(PIN_LDR);
#elif defined(ESP32)
  Uniot.configWiFiStatusLed(PIN_LED, LED_PIN_LEVEL);
  Uniot.registerLispDigitalOutput(PIN_LED, PIN_VIBRO);
  Uniot.registerLispDigitalInput(PIN_BUTTON);
#endif

  taskPrintHeap->attach(500);
  taskPrintTime->attach(500);

  Uniot.addLispPrimitive(filter_events);
  Uniot.begin();
}

void loop() {
  Uniot.loop();
}
