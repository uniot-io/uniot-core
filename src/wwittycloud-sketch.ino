#include <Uniot.h>
#include <Board-WittyCloud.h>
#include <CBOR.h>
#include <AppKit.h>
#include <Storage.h>
#include <LispPrimitives.h>
#include <Logger.h>

using namespace uniot;

AppKit MainAppKit(MyCredentials, PIN_BUTTON, LOW, RED);

auto taskPrintHeap = TaskScheduler::make([&](short t) {
  Serial.println(ESP.getFreeHeap());
});

void inject()
{
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  pinMode(LDR, INPUT);

  MainBroker.connect(&MainAppKit);
  MainScheduler.push(&MainAppKit)
      ->push(taskPrintHeap);

  taskPrintHeap->attach(500);

  MainAppKit.attach();
  MainAppKit.begin();

  UNIOT_LOG_INFO("%s: %s", "CHIP_ID", String(ESP.getChipId(), HEX).c_str());
  UNIOT_LOG_INFO("%s: %s", "DEVICE_ID", MyCredentials.getDeviceId().c_str());
  UNIOT_LOG_INFO("%s: %s", "OWNER_ID", MyCredentials.getOwnerId().c_str());
}
