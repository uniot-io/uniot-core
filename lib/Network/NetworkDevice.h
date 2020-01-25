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
#include <TaskScheduler.h>
#include <Button.h>

namespace uniot
{
class NetworkDevice : public IExecutor, public IBrokerKitConnection<int, int>
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
    _initTasks();
    _initSubscribers();
  }

  uint8_t execute()
  {
    return mNetwork.execute();
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

  NetworkScheduler &getNetworkScheduler()
  {
    return mNetwork;
  }

  void attach() {
    mTaskConfigBtn->attach(100);
  }

  void statusBusy()
  {
    mTaskSignalLed->attach(500);
  }

  void statusAlarm()
  {
    mTaskSignalLed->attach(200);
  }

  void statusIdle()
  {
    mTaskSignalLed->attach(200, 1);
  }

private:
  void _initTasks()
  {
    mNetwork.push(mTaskSignalLed = TaskScheduler::make([&](short t) {
      static bool pinLevel = true;
      digitalWrite(mPinLed, pinLevel = (!pinLevel && t));
    }));
    mNetwork.push(mTaskConfigBtn = TaskScheduler::make(mConfigBtn.getTaskCallback()));
  }

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

  TaskScheduler::TaskPtr mTaskSignalLed;
  TaskScheduler::TaskPtr mTaskConfigBtn;

  std::unique_ptr<Subscriber<int, int>> mpSubscriberNetwork;
};
} // namespace uniot
