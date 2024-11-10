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

void setup() {
  Uniot.begin();
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

  //   esp_chip_info_t chip_info;
  // esp_chip_info(&chip_info);

  // // Print basic chip information
  // Serial.println("====================================");
  // Serial.println("ESP32-C3 Chip Information:");
  // Serial.printf("Number of CPU cores: %d\n", chip_info.cores);
  // Serial.printf("WiFi%s%s\n",
  //               (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
  //               (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
  // Serial.printf("Silicon revision: %d\n", chip_info.revision);
  // Serial.printf("Flash size: %dMB\n", spi_flash_get_chip_size() / (1024 * 1024));
  // Serial.printf("Model: ESP32-C3\n"); // Assuming it's ESP32-C3; adjust if necessary
  // Serial.println("====================================");
}

void loop() {
  Uniot.loop();
}