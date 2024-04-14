#include <AppKit.h>
#include <Date.h>
#include <LispPrimitives.h>
#include <Logger.h>
#include <Uniot.h>

#define PIN_STATUS_LED 13
#define PIN_RELAY 12
#define PIN_BUTTON 0
#define BTN_PIN_LEVEL LOW

using namespace uniot;

auto taskPrintHeap = TaskScheduler::make([](short t) {
  Serial.println(ESP.getFreeHeap());
});

auto taskPrintTime = TaskScheduler::make([](short t) {
  Serial.println(Date::getFormattedTime());
});

void inject() {
  auto& MainAppKit = AppKit::getInstance(PIN_BUTTON, BTN_PIN_LEVEL, PIN_STATUS_LED);
  UniotPinMap.setDigitalOutput(2, PIN_RELAY, PIN_STATUS_LED);

  MainEventBus.registerKit(MainAppKit);

  MainScheduler
      .push(MainAppKit)
      .push("print_time", taskPrintTime)
      .push("print_heap", taskPrintHeap);

  taskPrintHeap->attach(500);
  taskPrintTime->attach(500);

  MainAppKit.attach();

  UNIOT_LOG_INFO("%s: %s", "CHIP_ID", String(ESP.getChipId(), HEX).c_str());
  UNIOT_LOG_INFO("%s: %s", "DEVICE_ID", MainAppKit.getCredentials().getDeviceId().c_str());
  UNIOT_LOG_INFO("%s: %s", "OWNER_ID", MainAppKit.getCredentials().getOwnerId().c_str());
}
