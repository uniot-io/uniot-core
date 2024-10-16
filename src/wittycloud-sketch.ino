#include <AppKit.h>
#include <Board-WittyCloud.h>
#include <Date.h>
#include <Logger.h>
#include <Uniot.h>

using namespace uniot;

auto taskPrintHeap = TaskScheduler::make([](SchedulerTask& self, short t) {
  Serial.println(ESP.getFreeHeap());
});

auto taskPrintTime = TaskScheduler::make([](SchedulerTask& self, short t) {
  Serial.println(Date::getFormattedTime());
});

void inject() {
  auto& MainAppKit = AppKit::getInstance();
#if defined(ESP8266)
  MainAppKit.configureNetworkController({.pinBtn = PIN_BUTTON, .pinLed = RED, .maxRebootCount = 255});
  PrimitiveExpeditor::getRegisterManager().setDigitalOutput(RED, GREEN, BLUE);
  PrimitiveExpeditor::getRegisterManager().setDigitalInput(0, PIN_BUTTON);
  PrimitiveExpeditor::getRegisterManager().setAnalogOutput(RED, GREEN, BLUE);
  PrimitiveExpeditor::getRegisterManager().setAnalogInput(LDR);
#elif defined(ESP32)
  MainAppKit.configureNetworkController({.pinBtn = 3, .pinLed = 8, .activeLevelLed = LOW, .maxRebootCount = 255});
  PrimitiveExpeditor::getRegisterManager().setDigitalOutput(8, 10);
  PrimitiveExpeditor::getRegisterManager().setDigitalInput(3);
#endif

  MainEventBus.registerKit(MainAppKit);

  MainScheduler
      .push(MainAppKit)
      .push("print_time", taskPrintTime)
      .push("print_heap", taskPrintHeap);

  taskPrintHeap->attach(500);
  taskPrintTime->attach(500);

  MainAppKit.attach();

  UNIOT_LOG_INFO("%s: %s", "DEVICE_ID", MainAppKit.getCredentials().getDeviceId().c_str());
  UNIOT_LOG_INFO("%s: %s", "OWNER_ID", MainAppKit.getCredentials().getOwnerId().c_str());
}
