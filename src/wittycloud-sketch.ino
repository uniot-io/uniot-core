#include <AppKit.h>
#include <Board-WittyCloud.h>
#include <Date.h>
#include <LispPrimitives.h>
#include <Logger.h>
#include <Uniot.h>

using namespace uniot;

auto taskPrintHeap = TaskScheduler::make([](short t) {
  Serial.println(ESP.getFreeHeap());
});

auto taskPrintTime = TaskScheduler::make([](short t) {
  Serial.println(Date::getFormattedTime());
});

void inject() {
  auto &MainAppKit = AppKit::getInstance(PIN_BUTTON, BTN_PIN_LEVEL, RED);
  UniotPinMap.setDigitalOutput(3, RED, GREEN, BLUE);
  UniotPinMap.setDigitalInput(3, RED, GREEN, BLUE);
  UniotPinMap.setAnalogOutput(3, RED, GREEN, BLUE);
  UniotPinMap.setAnalogInput(1, LDR);

  MainEventBus.registerKit(&MainAppKit);

  MainScheduler.push(&MainAppKit)
      ->push(taskPrintTime)
      ->push(taskPrintHeap);

  taskPrintHeap->attach(500);
  taskPrintTime->attach(500);

  MainAppKit.begin();

  UNIOT_LOG_INFO("%s: %s", "CHIP_ID", String(ESP.getChipId(), HEX).c_str());
  UNIOT_LOG_INFO("%s: %s", "DEVICE_ID", MainAppKit.getCredentials().getDeviceId().c_str());
  UNIOT_LOG_INFO("%s: %s", "OWNER_ID", MainAppKit.getCredentials().getOwnerId().c_str());
}
