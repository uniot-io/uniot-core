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

Object filter_events(Root root, VarObject env, VarObject list) {
  auto expeditor = PrimitiveExpeditor::describe("filter_events", Lisp::Bool, 1, Lisp::Symbol)
                       .init(root, env, list);
  expeditor.assertDescribedArgs();
  String events = expeditor.getArgSymbol(0);

  if (events.isEmpty()) {
    UNIOT_LOG_ERROR("No events provided for filtering");
    return expeditor.makeBool(false);
  }

  UNIOT_LOG_INFO("Filtering events: %s", events.c_str());

  // Create a queue to store allowed event names
  static ClearQueue<String> allowedEvents;
  allowedEvents.clean();  // Clear previous filters

  // Parse the input string and split by ':'
  int startPos = 0;
  int colonPos = events.indexOf(':');

  while (colonPos >= 0) {
    String eventName = events.substring(startPos, colonPos);
    if (eventName.length() > 0) {
      allowedEvents.push(eventName);
      UNIOT_LOG_INFO("Added event filter: %s", eventName.c_str());
    }
    startPos = colonPos + 1;
    colonPos = events.indexOf(':', startPos);
  }

  // Don't forget the last event after the last colon (or the only event if no colons)
  String lastEvent = events.substring(startPos);
  if (lastEvent.length() > 0) {
    allowedEvents.push(lastEvent);
    UNIOT_LOG_INFO("Added event filter: %s", lastEvent.c_str());
  }

  // Set the event filter in AppKit
  auto& appKit = AppKit::getInstance();
  appKit.setLispEventInterceptor([](const LispEvent& event) {
    // Check if this event is in our allowed list
    if (allowedEvents.contains(event.eventID)) {
      UNIOT_LOG_INFO("Allowing event: %s", event.eventID.c_str());
      return true;  // Allow this event
    }

    UNIOT_LOG_INFO("Filtering out event: %s", event.eventID.c_str());
    return false;  // Filter out this event
  });

  return expeditor.makeBool(true);
}

auto taskPrintHeap = TaskScheduler::make([](SchedulerTask& self, short t) {
  Serial.println(ESP.getFreeHeap());
});

auto taskPrintTime = TaskScheduler::make([](SchedulerTask& self, short t) {
  Serial.println(Date::getFormattedTime());
  AppKit::getInstance().publishLispEvent("time", Date::now());
});

void setup() {
  Serial.begin(115200);
  Uniot.configWiFiResetButton(PIN_BUTTON, BTN_PIN_LEVEL);
  Uniot.configWiFiStatusLed(PIN_RED, LED_PIN_LEVEL);
  Uniot.configWiFiResetOnReboot(255);  // Set max reboot count to 255
#if defined(ESP8266)
  PrimitiveExpeditor::getRegisterManager().setDigitalOutput(PIN_RED, PIN_GREEN, PIN_BLUE);
  PrimitiveExpeditor::getRegisterManager().setDigitalInput(0, PIN_BUTTON);
  PrimitiveExpeditor::getRegisterManager().setAnalogOutput(PIN_RED, PIN_GREEN, PIN_BLUE);
  PrimitiveExpeditor::getRegisterManager().setAnalogInput(PIN_LDR);
#elif defined(ESP32)
  MainAppKit.configureNetworkController({.pinBtn = PIN_BUTTON, .activeLevelBtn = BTN_PIN_LEVEL, .pinLed = PIN_LED, .activeLevelLed = LED_PIN_LEVEL, .maxRebootCount = 255});
  PrimitiveExpeditor::getRegisterManager().setDigitalOutput(PIN_LED, PIN_VIBRO);
  PrimitiveExpeditor::getRegisterManager().setDigitalInput(PIN_BUTTON);
#endif

  Uniot.getScheduler()
      .push("print_time", taskPrintTime)
      .push("print_heap", taskPrintHeap);

  taskPrintHeap->attach(500);
  taskPrintTime->attach(500);

  Uniot.addLispPrimitive(filter_events);

  auto interval = Uniot.setInterval([]() {
    Serial.println("Hello from setInterval!");
  }, 1000);

  Uniot.setTimeout([&]() {
    Serial.println("Hello from setTimeout!");
    interval.cancel();  // Cancel the interval after 15 seconds
  }, 60000);

  Uniot.begin();
}

void loop() {
  Uniot.loop();
}