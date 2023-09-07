/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2020 Uniot <contact@uniot.io>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <EventBus.h>
#include <Date.h>
#include <NetworkDevice.h>
#include <CallbackEventListener.h>
#include <MQTTKit.h>
#include <unLisp.h>
#include <IEventBusKitConnection.h>
#include <ISchedulerKitConnection.h>
#include <Logger.h>
#include <LispDevice.h>
#include <LispPrimitives.h>
#include <CrashStorage.h>
#include <PinMap.h>

namespace uniot
{
class AppKit : public ICoreEventBusKitConnection, public ISchedulerKitConnection
{
public:
  AppKit(Credentials &credentials, uint8_t pinBtn, uint8_t activeLevelBtn, uint8_t pinLed)
      : mMQTT(credentials, [this, &credentials](CBORObject &info) {
          auto arr = info.putArray("primitives");
          getLisp().serializeNamesOfPrimitives(arr.get());
          arr->closeArray();

          info.put("creator", credentials.getCreatorId().c_str());
          info.put("mqtt_size", MQTT_MAX_PACKET_SIZE);
          info.put("d_in", UniotPinMap.getDigitalInputLength());
          info.put("d_out", UniotPinMap.getDigitalOutputLength());
          info.put("a_in", UniotPinMap.getAnalogInputLength());
          info.put("a_out", UniotPinMap.getAnalogOutputLength());
        }),
        mLispButton(pinBtn, activeLevelBtn, 30), mNetworkDevice(credentials, pinBtn, activeLevelBtn, pinLed)
  {
    _initMqtt();
    _initTasks();
    _initSubscribers();
    _initPrimitives();
  }

  static Date &getDate()
  {
    static Date date;
    return date;
  }

  unLisp &getLisp()
  {
    return unLisp::getInstance();
  }

  MQTTKit &getMQTT()
  {
    return mMQTT;
  }

  void begin()
  {
    attach();
    mNetworkDevice.begin();
    mLispDevice.runStoredCode();
  }

  void pushTo(TaskScheduler *scheduler) override
  {
    scheduler->push(&mNetworkDevice);
    scheduler->push(mTaskMQTT);
    scheduler->push(mTaskLispButton);
    scheduler->push(getLisp().getTask());
    scheduler->push(&getDate());
  }

  void attach() override
  {
    getDate().attach();
    mNetworkDevice.attach();
    mTaskLispButton->attach(100);
  }

  void connect(CoreEventBus *eventBus) override
  {
    eventBus->connect(&mNetworkDevice);
    eventBus->connect(&getLisp());
    eventBus->connect(&getDate());
    eventBus->connect(&mLispDevice);
    eventBus->connect(mpNetworkEventListener->listenToEvent(NetworkScheduler::CONNECTION));
  }

  void disconnect(CoreEventBus *eventBus) override
  {
    eventBus->disconnect(&mNetworkDevice);
    eventBus->disconnect(&getLisp());
    eventBus->disconnect(&getDate());
    eventBus->disconnect(&mLispDevice);
    eventBus->disconnect(mpNetworkEventListener->stopListeningToEvent(NetworkScheduler::CONNECTION));
  }

private:
  void _initMqtt()
  {
    // TODO: should I move configs to the Credentials class?
    mMQTT.setServer("mqtt.uniot.io", 1883);
    mMQTT.addDevice(&mLispDevice, "script");
  }

  void _refreshMqttDevices()
  {
    mLispDevice.unsubscribeFromAll();
    mLispDevice.subscribeDevice("script");
  }

  void _initTasks()
  {
    mTaskMQTT = TaskScheduler::make(&mMQTT);
    mTaskLispButton = TaskScheduler::make(mLispButton.getTaskCallback());
  }

  void _initPrimitives()
  {
    getLisp().pushPrimitive(globalPrimitive(dwrite));
    getLisp().pushPrimitive(globalPrimitive(dread));
    getLisp().pushPrimitive(globalPrimitive(awrite));
    getLisp().pushPrimitive(globalPrimitive(aread));
    getLisp().pushPrimitive(globalPrimitive(bclicked));

    PrimitiveExpeditor::getGlobalRegister().link("bclicked", &mLispButton);
  }

  void _initSubscribers()
  {
    mpNetworkEventListener = std::unique_ptr<CoreEventListener>(new CoreCallbackEventListener([&](int topic, int msg) {
      if (NetworkScheduler::CONNECTION == topic)
      {
        switch (msg)
        {
        case NetworkScheduler::SUCCESS:
          Serial.print("AppKit Subscriber, ip: ");
          Serial.println(WiFi.localIP());
          mTaskMQTT->attach(10);
          _refreshMqttDevices();
          break;
        case NetworkScheduler::ACCESS_POINT:
          Serial.println("AppKit Subscriber, ACCESS_POINT ");
          mTaskMQTT->detach();
          break;

        case NetworkScheduler::CONNECTING:
          Serial.println("AppKit Subscriber, CONNECTING ");
          mTaskMQTT->detach();
          break;

        case NetworkScheduler::DISCONNECTED:
          Serial.println("AppKit Subscriber, DISCONNECTED ");
          mTaskMQTT->detach();
          break;

        case NetworkScheduler::FAILED:
        default:
          Serial.println("AppKit Subscriber, FAILED ");
          mTaskMQTT->detach();
          break;
        }
      }
    }));
  }

  MQTTKit mMQTT;
  LispDevice mLispDevice;
  Button mLispButton;

  NetworkDevice mNetworkDevice;

  TaskScheduler::TaskPtr mTaskMQTT;
  TaskScheduler::TaskPtr mTaskLispButton;

  UniquePointer<CoreEventListener> mpNetworkEventListener;
};
} // namespace uniot
