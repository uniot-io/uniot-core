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

#include <Broker.h>
#include <NetworkDevice.h>
#include <CallbackSubscriber.h>
#include <MQTTKit.h>
#include <unLisp.h>
#include <IBrokerKitConnection.h>
#include <ISchedulerKitConnection.h>
#include <Logger.h>
#include <LispDevice.h>
#include <LispPrimitives.h>

namespace uniot
{
class AppKit : public IGeneralBrokerKitConnection, public ISchedulerKitConnection
{
public:
  AppKit(Credentials &credentials, uint8_t pinBtn, uint8_t activeLevelBtn, uint8_t pinLed)
      : mMQTT(credentials, [this](CBOR &info) {
          auto arr = info.putArray("primitives");
          getLisp().serializeNamesOfPrimitives(arr.get());
          arr->closeArray();
        }),
        mLispButton(pinBtn, activeLevelBtn, 30), mNetworkDevice(credentials, pinBtn, activeLevelBtn, pinLed)
  {
    _initMqtt();
    _initTasks();
    _initSubscribers();
    _initPrimitives();
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
    mNetworkDevice.begin();
  }

  void pushTo(TaskScheduler *scheduler)
  {
    scheduler->push(&mNetworkDevice);
    scheduler->push(mTaskMQTT);
    scheduler->push(mTaskLispButton);
    scheduler->push(getLisp().getTask());
  }

  void attach()
  {
    mNetworkDevice.attach();
    mTaskLispButton->attach(100);
  }

  void connect(GeneralBroker *broker)
  {
    broker->connect(&mNetworkDevice);
    broker->connect(&getLisp());
    broker->connect(mpSubscriberNetwork->subscribe(NetworkScheduler::CONNECTION));
    broker->connect(mpSubscriberLisp->subscribe(unLisp::LISP));
  }

  void disconnect(GeneralBroker *broker)
  {
    broker->disconnect(&mNetworkDevice);
    broker->disconnect(&getLisp());
    broker->disconnect(mpSubscriberNetwork->unsubscribe(NetworkScheduler::CONNECTION));
    broker->disconnect(mpSubscriberLisp->unsubscribe(unLisp::LISP));
  }

private:
  void _initMqtt()
  {
    mMQTT.setServer("mqtt.uniot.io", 1883);
    mMQTT.addDevice(&mLispDevice, "script");
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
    mpSubscriberNetwork = std::unique_ptr<GeneralSubscriber>(new GeneralCallbackSubscriber([&](int topic, int msg) {
      if (NetworkScheduler::CONNECTION == topic)
      {
        switch (msg)
        {
        case NetworkScheduler::SUCCESS:
          Serial.print("AppKit Subscriber, ip: ");
          Serial.println(WiFi.localIP());
          mTaskMQTT->attach(10);
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
    mpSubscriberLisp = std::unique_ptr<GeneralSubscriber>(new GeneralCallbackSubscriber([&](int topic, int msg) {
      if (msg == unLisp::ERROR)
      {
        auto lastError = getLisp().getLastError().c_str();
        UNIOT_LOG_ERROR("lisp error: %s", lastError);
      }
      else if (msg == unLisp::MSG_ADDED)
      {
        auto size = getLisp().sizeOutput();
        auto result = getLisp().popOutput();
        UNIOT_LOG_INFO("%s, %d msgs to read; %s", msg == unLisp::MSG_ADDED ? "added" : "replaced", size, result.c_str());
      }
    }));
  }

  MQTTKit mMQTT;
  LispDevice mLispDevice;
  Button mLispButton;

  NetworkDevice mNetworkDevice;

  TaskScheduler::TaskPtr mTaskMQTT;
  TaskScheduler::TaskPtr mTaskLispButton;

  UniquePointer<GeneralSubscriber> mpSubscriberNetwork;
  UniquePointer<GeneralSubscriber> mpSubscriberLisp;
};
} // namespace uniot
