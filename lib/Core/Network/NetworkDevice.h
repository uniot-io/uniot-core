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
#include <NetworkScheduler.h>
#include <CallbackSubscriber.h>
#include <IBrokerKitConnection.h>
#include <ISchedulerKitConnection.h>
#include <Button.h>

namespace uniot
{
class NetworkDevice : public IGeneralBrokerKitConnection, public ISchedulerKitConnection
{
public:
  NetworkDevice(Credentials &credentials, uint8_t pinBtn, uint8_t activeLevelBtn, uint8_t pinLed)
      : mNetwork(credentials),
        mClickCounter(0),
        mPinLed(pinLed),
        mConfigBtn(pinBtn, activeLevelBtn, 30, [&](Button *btn, Button::Event event) {
          switch (event)
          {
          case Button::LONG_PRESS:
            if (mClickCounter > 3)
              mNetwork.forget();
            else
              mNetwork.reconnect();
            break;
          case Button::CLICK:
            if (!mClickCounter)
              mpTaskResetClickCounter->attach(5000, 1);
            mClickCounter++;
          default:
            break;
          }
        })
  {
    _initSubscribers();
  }

  NetworkScheduler &getNetworkScheduler()
  {
    return mNetwork;
  }

  void connect(GeneralBroker *broker)
  {
    broker->connect(&mNetwork);
    broker->connect(mpSubscriberNetwork->subscribe(NetworkScheduler::CONNECTION));
  }

  void disconnect(GeneralBroker *broker)
  {
    broker->disconnect(&mNetwork);
    broker->disconnect(mpSubscriberNetwork->unsubscribe(NetworkScheduler::CONNECTION));
  }

  void pushTo(TaskScheduler *scheduler) {
    scheduler->push(mpTaskNetwork = TaskScheduler::make(&mNetwork));
    scheduler->push(mpTaskSignalLed = TaskScheduler::make([&](short t) {
      static bool pinLevel = true;
      digitalWrite(mPinLed, pinLevel = (!pinLevel && t));
    }));
    scheduler->push(mpTaskConfigBtn = TaskScheduler::make(mConfigBtn.getTaskCallback()));
    scheduler->push(mpTaskResetClickCounter = TaskScheduler::make([&](short t) {
      Serial.print("ClickCounter = ");
      Serial.println(mClickCounter);
      mClickCounter = 0;
    }));
  }

  void attach()
  {
    mpTaskNetwork->attach(1);
    mpTaskConfigBtn->attach(100);
  }

  void begin()
  {
    mNetwork.begin();
    statusBusy();
  }

  void statusWaiting()
  {
    mpTaskSignalLed->attach(1000);
  }

  void statusBusy()
  {
    mpTaskSignalLed->attach(500);
  }

  void statusAlarm()
  {
    mpTaskSignalLed->attach(200);
  }

  void statusIdle()
  {
    mpTaskSignalLed->attach(200, 1);
  }

private:
  void _initSubscribers()
  {
    mpSubscriberNetwork = std::unique_ptr<GeneralSubscriber>(new GeneralCallbackSubscriber([&](int topic, int msg) {
      if (NetworkScheduler::CONNECTION == topic)
      {
        switch (msg)
        {
        case NetworkScheduler::ACCESS_POINT:
          Serial.println("NetworkDevice Subscriber, ACCESS_POINT ");
          statusWaiting();
          break;

        case NetworkScheduler::SUCCESS:
          Serial.print("NetworkDevice Subscriber, ip: ");
          Serial.println(WiFi.localIP());
          statusIdle();
          break;

        case NetworkScheduler::CONNECTING:
          Serial.println("NetworkDevice Subscriber, CONNECTING ");
          statusBusy();
          break;

        case NetworkScheduler::DISCONNECTED:
          Serial.println("NetworkDevice Subscriber, DISCONNECTED ");
          mNetwork.reconnect();
          break;

        case NetworkScheduler::FAILED:
          Serial.println("NetworkDevice Subscriber, FAILED ");
        default:
          statusAlarm();
          break;
        }
      }
    }));
  }

  NetworkScheduler mNetwork;

  uint8_t mClickCounter;
  uint8_t mPinLed;
  Button mConfigBtn;

  TaskScheduler::TaskPtr mpTaskNetwork;
  TaskScheduler::TaskPtr mpTaskSignalLed;
  TaskScheduler::TaskPtr mpTaskConfigBtn;
  TaskScheduler::TaskPtr mpTaskResetClickCounter;

  UniquePointer<GeneralSubscriber> mpSubscriberNetwork;
};
} // namespace uniot
