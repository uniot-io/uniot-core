#include <AppKit.h>
#include <Date.h>
#include <Logger.h>
#include <Uniot.h>

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

auto taskPrintHeap = TaskScheduler::make([](SchedulerTask& self, short t) {
  Serial.println(ESP.getFreeHeap());
});

auto taskPrintTime = TaskScheduler::make([](SchedulerTask& self, short t) {
  Serial.println(Date::getFormattedTime());
});

void setup() {
  Uniot.begin();
  auto& MainAppKit = AppKit::getInstance();
#if defined(ESP8266)
  MainAppKit.configureNetworkController({.pinBtn = PIN_BUTTON, .activeLevelBtn = BTN_PIN_LEVEL, .pinLed = PIN_RED, .activeLevelLed = LED_PIN_LEVEL, .maxRebootCount = 255});
  PrimitiveExpeditor::getRegisterManager().setDigitalOutput(PIN_RED, PIN_GREEN, PIN_BLUE);
  PrimitiveExpeditor::getRegisterManager().setDigitalInput(0, PIN_BUTTON);
  PrimitiveExpeditor::getRegisterManager().setAnalogOutput(PIN_RED, PIN_GREEN, PIN_BLUE);
  PrimitiveExpeditor::getRegisterManager().setAnalogInput(PIN_LDR);
#elif defined(ESP32)
  MainAppKit.configureNetworkController({.pinBtn = PIN_BUTTON, .activeLevelBtn = BTN_PIN_LEVEL, .pinLed = PIN_LED, .activeLevelLed = LED_PIN_LEVEL, .maxRebootCount = 255});
  PrimitiveExpeditor::getRegisterManager().setDigitalOutput(PIN_LED, PIN_VIBRO);
  PrimitiveExpeditor::getRegisterManager().setDigitalInput(PIN_BUTTON);
#endif

  Uniot.getEventBus().registerKit(MainAppKit);

  Uniot.getScheduler()
      .push(MainAppKit)
      .push("print_time", taskPrintTime)
      .push("print_heap", taskPrintHeap);

  taskPrintHeap->attach(500);
  taskPrintTime->attach(500);

  MainAppKit.attach();

  UNIOT_LOG_INFO("%s: %s", "DEVICE_ID", MainAppKit.getCredentials().getDeviceId().c_str());
  UNIOT_LOG_INFO("%s: %s", "OWNER_ID", MainAppKit.getCredentials().getOwnerId().c_str());
}

void loop() {
  Uniot.loop();
}