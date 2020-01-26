/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2020 Uniot <info.uniot@gmail.com>
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

#include <NetworkScheduler.h>
#include <Broker.h>
#include <CallbackSubscriber.h>
#include <IBrokerKitConnection.h>
#include <ISchedulerKitConnection.h>
#include <TaskScheduler.h>
#include <Button.h>

namespace uniot
{
class NetworkDevice : public IBrokerKitConnection<int, int>, public ISchedulerKitConnection
{
public:
  NetworkDevice(uint8_t pinBtn, uint8_t activeLevelBtn, uint8_t pinLed)
      : mPinLed(pinLed),
        mConfigBtn(pinBtn, activeLevelBtn, 30, [&](Button *btn, Button::Event event) {
          switch (event)
          {
          case Button::LONG_PRESS:
            mNetwork.reconnect();
            statusBusy();
            break;
          case Button::CLICK:
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

  void connect(Broker<int, int> *broker)
  {
    broker->connect(&mNetwork);
    broker->connect(mpSubscriberNetwork->subscribe(NetworkScheduler::CONNECTION));
  }

  void disconnect(Broker<int, int> *broker)
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
    mpSubscriberNetwork = std::unique_ptr<Subscriber<int, int>>(new CallbackSubscriber<int, int>([&](int topic, int msg) {
      if (NetworkScheduler::CONNECTION == topic)
      {
        switch (msg)
        {
        case NetworkScheduler::SUCCESS:
          Serial.print("NetworkDevice Subscriber, ip: ");
          Serial.println(WiFi.localIP());
          statusIdle();
          break;

        case NetworkScheduler::CONNECTING:
          statusBusy();
          break;

        case NetworkScheduler::DISCONNECTED:
          mNetwork.reconnect();
          break;

        case NetworkScheduler::FAILED:
        default:
          statusAlarm();
          break;
        }
      }
    }));
  }

  uint8_t mPinLed;
  Button mConfigBtn;

  NetworkScheduler mNetwork;

  TaskScheduler::TaskPtr mpTaskNetwork;
  TaskScheduler::TaskPtr mpTaskSignalLed;
  TaskScheduler::TaskPtr mpTaskConfigBtn;

  std::unique_ptr<Subscriber<int, int>> mpSubscriberNetwork;
};
} // namespace uniot
