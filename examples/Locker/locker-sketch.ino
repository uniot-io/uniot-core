#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AppKit.h>
#include <Board-WittyCloud.h>
#include <Date.h>
#include <LispPrimitives.h>
#include <Logger.h>
#include <Uniot.h>

using namespace uniot;

#define RELAY_PIN D3
#define OLED_RESET 0
Adafruit_SSD1306 display(OLED_RESET);

auto taskPrintHeap = TaskScheduler::make([](short t) {
  Serial.println(ESP.getFreeHeap());
});

auto taskCheckRelay = TaskScheduler::make([](short t) {
  if (digitalRead(RELAY_PIN)) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(9, 16);
    display.println("OPEN");
    display.display();
  } else {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(3, 16);
    display.println("CLOSE");
    display.display();
  }
});

void inject() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();

  auto &MainAppKit = AppKit::getInstance(PIN_BUTTON, BTN_PIN_LEVEL, D4);
  UniotPinMap.setDigitalOutput(2, RELAY_PIN, D4);
  digitalWrite(RELAY_PIN, LOW);
  MainEventBus.registerKit(&MainAppKit);

  MainScheduler.push(&MainAppKit)
      ->push(taskCheckRelay)
      ->push(taskPrintHeap);

  taskPrintHeap->attach(500);
  taskCheckRelay->attach(50);

  MainAppKit.begin();

  UNIOT_LOG_INFO("%s: %s", "CHIP_ID", String(ESP.getChipId(), HEX).c_str());
  UNIOT_LOG_INFO("%s: %s", "DEVICE_ID", MainAppKit.getCredentials().getDeviceId().c_str());
  UNIOT_LOG_INFO("%s: %s", "OWNER_ID", MainAppKit.getCredentials().getOwnerId().c_str());
}
