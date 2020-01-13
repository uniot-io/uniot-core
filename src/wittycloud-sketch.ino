#ifndef _SKETCH_SELECTED_
#define _SKETCH_SELECTED_

// uLisp
// (def odd (n) (= 1 (% n 2))) (def color (n) (% n 3)) (task 30 200 '(led (color (t-get env 't_pass)) (odd (t-get env 't_pass))))
// (def color (n) (% n 3))(task 0 500 '( (print (ldr)) (led 0 0) (led 1 0) (led 2 0) (led ( - (/ (ldr) 100) 1) 1)))

// unLisp
// (defun odd (n) (= 1 (% n 2))) (defun color (n) (% n 3)) (task 30 200 '(led (color #t_pass) (odd #t_pass)))
// (defun color (n) (% n 3))(task 0 500 '(list (print (ldr)) (led 0 0) (led 1 0) (led 2 0) (led ( - (/ (ldr) 100) 1) 1)))

#include <Wire.h>
#include <Arduino.h>
#include <Uniot.h>
#include <Board-WittyCloud.h>
#include <NetworkScheduler.h>
#include <MQTTKit.h>
#include <Broker.h>
#include <CallbackSubscriber.h>
#include <unLisp.h>
#include <Bytes.h>
#include <CBOR.h>
#include <Button.h>

using namespace uniot;

NetworkScheduler Network;

Broker<int, int> MainBrocker;
String DeviceId = String(ESP.getChipId(), HEX); // TODO: CBOR: implement storage for dynamic values 

MQTTDevice mqttDevice([](const String &topic, const Bytes &pa) {
  Serial.println(topic);
  Serial.write(pa.raw(), pa.size());
  Serial.println();

  if (topic.endsWith("script")) {
    unLisp::getInstance().runCode(pa);
  }

  if (topic.endsWith("online/request")) {
    CBOR packet;
    packet.put("id", DeviceId.c_str()); // TODO: CBOR: implement storage for dynamic values
    packet.put("type", "rgb");
    mqttDevice.publish("bits/TEST/online/response", packet.write());
  }
});
MQTTKit mqtt;

auto taskHandleBroker = TaskScheduler::make(&MainBrocker);
auto taskNetwork = TaskScheduler::make(&Network);
auto taskPrintHeap = TaskScheduler::make([&](short t) {
  PRINT_HEAP();
  // Serial.println(WiFi.status());
  // Serial.println(analogRead(LDR));
});
auto taskSignalLed = TaskScheduler::make([](short t) {
  static bool pinLevel = true;
  digitalWrite(BLUE, pinLevel = (!pinLevel && t));
});
auto taskMQTT = TaskScheduler::make(&mqtt);

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
  static bool state = false;
  switch(event) {
    case Button::LONG_PRESS: Network.reconnect(); statusBusy(); break;
    case Button::CLICK: digitalWrite(BLUE, state = !state); break;
    default: break;
  }
});

auto taskCheckBtn = TaskScheduler::make(SetupButton.getTaskCallback());

CallbackSubscriber<int, int> MainSubscriber([&](int topic, int msg) {
  if(NetworkScheduler::CONNECTION == topic) {
    switch(msg) {
      case NetworkScheduler::SUCCESS:
      Serial.println(WiFi.localIP());
      statusIdle();
      taskMQTT->attach(10);
      break;

      case NetworkScheduler::CONNECTING:
      taskMQTT->detach();
      statusBusy();
      break;

      case NetworkScheduler::DISCONNECTED:
      taskMQTT->detach();
      Network.reconnect();
      break;

      case NetworkScheduler::FAILED:
      default: 
      taskMQTT->detach();
      statusAlarm(); 
      break;
    }
  }
});

auto mLispSubscriber = std::unique_ptr<Subscriber<int, int>>(new CallbackSubscriber<int, int>([&](int topic, int msg) {
  auto size = unLisp::getInstance().sizeOutput();
  auto result = unLisp::getInstance().popOutput();
  if (msg == unLisp::ADDED)
  {
    Serial.println("ADDED: " + String(size) + " : " + result);
  }
  else
  {
    Serial.println("REPLACED: " + String(size) + " : " + result);
  }
}));

struct Obj *user_prim_led(void *root, struct Obj **env, struct Obj **list)
{
  auto args = eval_list(root, env, list);
  if (length(args) != 2)
    error("Malformed task");

  auto objColor = args->car;
  auto objState = args->cdr->car;

  // TODO: check types
  // TODO: allow bool too

  auto color = objColor->value;
  auto state = objState != Nil;
  if (objState->type == TINT)
  {
    state = objState->value != 0;
  }

  uint8_t pin = 0;
  switch (color)
  {
  case 0:
    pin = RED;
    break;
  case 1:
    pin = GREEN;
    break;
  case 2:
    pin = BLUE;
    break;
  default:
    error("Out of range: color: [0, 1, 2]");
  }

  digitalWrite(pin, state);
  return objState;
}

struct Obj *user_prim_ldr(void *root, struct Obj **env, struct Obj **list)
{
  DEFINE1(objLdr);
  *objLdr = make_int(root, analogRead(LDR));
  return *objLdr;
}

void setup()
{
  Serial.begin(9600);

  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  pinMode(LDR, INPUT);

  mqtt.setServer("broker.hivemq.com", 1883);
  mqtt.addDevice(&mqttDevice);
  mqttDevice.subscribe("bits/TEST/online/request");
  mqttDevice.subscribe("bits/TEST/script");

  MainBrocker.connect(&Network);
  MainBrocker.connect(MainSubscriber.subscribe(NetworkScheduler::CONNECTION));
  MainBrocker.connect(&unLisp::getInstance());
  MainBrocker.connect(mLispSubscriber->subscribe(unLisp::OUTPUT_BUF));

  Scheduler.push(taskNetwork)
      ->push(taskCheckBtn)
      ->push(taskHandleBroker)
      ->push(taskPrintHeap)
      ->push(taskSignalLed)
      ->push(taskMQTT)
      ->push(unLisp::getInstance().getTask())
      ;

  taskNetwork->attach(1);
  taskCheckBtn->attach(100);
  taskHandleBroker->attach(500);
  taskPrintHeap->attach(500);

  Network.begin();
  statusBusy();

  unLisp::getInstance().pushPrimitive("led", user_prim_led);
  unLisp::getInstance().pushPrimitive("ldr", user_prim_ldr);
}

#endif // _SKETCH_SELECTED_
