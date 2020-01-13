#include <Arduino.h>
#include <Uniot.h>
#include <Sonoff.h>
#include <WiFiUdp.h>
#include <NetworkScheduler.h>
#include <CoAPKit.h>
#include <CoAPRelay.h>
#include <Broker.h>
#include <CallbackSubscriber.h>
#include <Button.h>

using namespace uniot;

WiFiUDP Udp;
NetworkScheduler Network;

CoAPKit Coap(&Udp);
CoAPRelay RelayDevice(PIN_RELAY, "relay");
Broker<int, int> MainBrocker;

auto taskHandleBroker = TaskScheduler::make(&MainBrocker);
auto taskNetwork = TaskScheduler::make(&Network);
auto taskCoAP = TaskScheduler::make(&Coap);
auto taskPrintHeap = TaskScheduler::make([&](short t) { PRINT_HEAP(); Serial.println(WiFi.status());});
auto taskSignalLed = TaskScheduler::make([](short t) {
  static bool pinLevel = true;
  digitalWrite(PIN_STATUS_LED, pinLevel = (!pinLevel && t));
});

void statusBusy() {
  taskSignalLed->attach(500);
}

void statusAlarm() {
  taskSignalLed->attach(200);
}

void statusIdle() {
  taskSignalLed->attach(200, 1);
}

Button SetupButton(PIN_BUTTON, LOW, 30, [](Button* btn, Button::Event event) {
  switch(event) {
    case Button::LONG_PRESS: Network.reconnect(); statusBusy(); break;
    case Button::CLICK: RelayDevice.setState(!RelayDevice.getState()); break;
    default: break;
  }
});

auto taskCheckBtn = TaskScheduler::make(SetupButton.getTaskCallback());

CallbackSubscriber<int, int> MainSubscriber([&](int topic, int msg) {
  if(NetworkScheduler::CONNECTION == topic) {
    switch(msg) {
      case NetworkScheduler::SUCCESS:
      Serial.println(WiFi.localIP());
      Coap.begin(WiFi.subnetMask());
      statusIdle();
      break;

      case NetworkScheduler::CONNECTING:
      statusBusy();
      break;

      case NetworkScheduler::DISCONNECTED:
      Network.reconnect();
      break;

      case NetworkScheduler::FAILED:
      default: 
      statusAlarm(); 
      break;
    }
  }
});

void setup() {
  pinMode(PIN_STATUS_LED, OUTPUT);
  Serial.begin(9600);

  Scheduler
  .push(taskNetwork)
  ->push(taskCoAP)
  ->push(taskCheckBtn)
  ->push(taskHandleBroker)
  ->push(taskPrintHeap)
  ->push(taskSignalLed)
  ;

  taskNetwork->attach(1);
  taskCoAP->attach(10);
  taskCheckBtn->attach(100);
  taskHandleBroker->attach(500);
  taskPrintHeap->attach(500);

  MainBrocker.connect(&Network);
  MainBrocker.connect(MainSubscriber.subscribe(NetworkScheduler::CONNECTION));

  RelayDevice.connect(&Coap);
  Network.begin();
  statusBusy();
}
