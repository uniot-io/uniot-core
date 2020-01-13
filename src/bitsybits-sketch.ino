#ifndef _SKETCH_SELECTED_
#define _SKETCH_SELECTED_

#include <Wire.h>
#include <Arduino.h>
#include <BitsyBits.h>
#include <Generic.h>
// #include <Motherboard.h>
// #include <LolinV3.h>
#include <NetworkScheduler.h>
#include <MQTTKit.h>

#include <Broker.h>
#include <CallbackSubscriber.h>
#include <uLisp.h>
#include <Bytes.h>

#include <EthRpc.h>
#include <Ethereum.h>
#include <SimpleDeviceMap.h>

Bytes ethAddress("0xc91E046D01445A37dC7e5BC839d74B50DfE286e7");
Bytes ethDevice((uint8_t *)"TEST", 4);

SSD1306 mDisplay(&Wire);
ConsoleView mConsoleView(&mDisplay);
ConsoleController mConsole(&mConsoleView);
bits::esp::NetworkScheduler mNetworkScheduler(&mConsole);
DPad mDpad(DPAD_UP, DPAD_CENTER, DPAD_DOWN, LOW, DPAD_PIN_MODE);
Broker<int, int> mBroker;

EthRpc mEthRpc;
Ethereum mEth(contract::SimpleDeviceMap::getCommand());

String mScript = "(def odd (n) (= 1 (\% n 2)))(task 10 500 '(led 0 (odd (t-get env 't_pass))))";

MQTTDevice mqttDevice([](char *topic, const Bytes &pa) {
  Serial.println(topic);
  Serial.write(pa.raw(), pa.size());
  Serial.println();
  uLisp::getInstance().runCode(pa.c_str());
});
MQTTKit mqtt;

bool mConnected = false;

auto task = bits::esp::TaskScheduler::make([&](short t) { PRINT_HEAP(); });
auto taskConsole = bits::esp::TaskScheduler::make(&mConsole);
auto taskHandle = bits::esp::TaskScheduler::make(&mBroker);
auto taskNetwork = bits::esp::TaskScheduler::make(&mNetworkScheduler);
auto taskVibroOnce = bits::esp::TaskScheduler::make([&](short t) { digitalWrite(VIBRO, (bool)t == (bool)VIBRO_LEVEL); });
auto taskMQTT = bits::esp::TaskScheduler::make(&mqtt);
auto taskEthereum = bits::esp::TaskScheduler::make([&](short t) {
  auto slicedTime = millis();
  // mScript = mEth.decodeResult(mEthRpc.ethCall(ethAddress, mEth.encodeRequest(ethDevice)));
  Serial.println(mScript);
  uLisp::getInstance().runCode(mScript.c_str());
  slicedTime = millis() - slicedTime;
  mConsole.printlnString("Eth call " + String(slicedTime, 10) + "ms");
  taskVibroOnce->attach(150, 2);
});
auto taskDpad = bits::esp::TaskScheduler::make([&](short t) {
  if (mDpad.isUp()) {
    mConsoleView.incRow();
  }
  if (mDpad.isDown()) {
    mConsoleView.decRow();
  }
  if (mDpad.isCenter()) {
    if (mConnected) {
      taskVibroOnce->attach(100, 2);
      taskEthereum->attach(500, 1);
    } else {
      mNetworkScheduler.reconnect();
    }
  }
});

auto mNetworkSubscriber = std::unique_ptr<Subscriber<int, int>>
(new CallbackSubscriber<int, int>([&](int topic, int msg) {
  if(NetworkScheduler::CONNECTION == topic) {
    switch(msg) {
      case NetworkScheduler::SUCCESS: {
        auto ip = WiFi.localIP().toString();
        WiFi.subnetMask();
        IPAddress broadcastIp = ~uint32_t(WiFi.subnetMask()) | uint32_t(WiFi.gatewayIP());
        mConsole.printlnString(ip);
        mConsole.printlnString(broadcastIp.toString());

        mConnected = true;

        taskMQTT->attach(10);
        break;
      }
      case NetworkScheduler::DISCONNECTED: {
        mNetworkScheduler.reconnect();
        break;
      }
      case NetworkScheduler::FAILED: {
        mConsole.printlnString("Network down!");
        mConnected = false;
      }
      default: break;
    }
  }
}));

auto mLispSubscriber = std::unique_ptr<Subscriber<int, int>>
(new CallbackSubscriber<int, int>([&](int topic, int msg) {
  auto size = uLisp::getInstance().sizeOutput();
  auto result = uLisp::getInstance().popOutput();
  Serial.println(result.length());
  mConsole.printlnString("Lisp: " + result);
  if(msg == uLisp::ADDED) {
    Serial.println("ADDED: " + String(size) + " : " + result);
  } else {
    Serial.println("REPLACED: " + String(size) + " : " + result);
  }
}));

builtin_t mLedBuiltin = {"led", [](value_t args) { 
  value_t pin = next(&args);
  if (pin.type != IntegerType) {
    return TypeError;
  }
  value_t state = next(&args);
  if (state.type != IntegerType && state.type != AtomType) {
    return TypeError;
  }
  uint8_t realPin = 0;
  switch (pin.data) {
    case 0:
      realPin = OUT0;
      break;
    default:
      return RangeError;
  }

  digitalWrite(realPin, state.data);
  return state;
}};

void setup() {
  Serial.begin(9600);
  Wire.begin(SDA, SCL);
  Wire.setClock(400000);
  pinMode(VIBRO, OUTPUT);
  pinMode(OUT0, OUTPUT);

  mqtt.setServer("mqtt.eclipse.org", 1883);
  mqtt.addDevice(&mqttDevice);
  mqttDevice.subscribe("myInTopic");

  mBroker.connect(&mNetworkScheduler);
  mBroker.connect(mNetworkSubscriber->subscribe(bits::esp::NetworkScheduler::CONNECTION));
  mBroker.connect(&uLisp::getInstance());
  mBroker.connect(mLispSubscriber->subscribe(uLisp::OUTPUT_BUF));

  mDpad.init();
  
  mDisplay.init();
  mDisplay.clear();
  mDisplay.display();
  mDisplay.flipScreenVertically();
  mDisplay.setFontScale2x2(false);

  Scheduler.push(taskConsole)
  ->push(taskNetwork)
  ->push(taskHandle)
  ->push(taskVibroOnce)
  ->push(taskEthereum)
  ->push(taskDpad)
  ->push(taskMQTT)
  ->push(uLisp::getInstance().getTask())
  ;

  taskConsole->attach(200);
  taskNetwork->attach(1);
  taskHandle->attach(500);
  taskDpad->attach(50);
  taskVibroOnce->attach(50, 2);
 
  mNetworkScheduler.begin();

  uLisp::getInstance().resetBuiltins();
  uLisp::getInstance().pushBuiltin(&mLedBuiltin);
  uLisp::getInstance().buildBuiltins();
}

#endif // _SKETCH_SELECTED_
