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

#include <MQTTKit.h>
#include <CallbackSubscriber.h>
#include <NetworkDevice.h>
#include <Broker.h>
#include <unLisp.h>
#include <IBrokerKitConnection.h>

namespace uniot
{
class AppKit : public IBrokerKitConnection<int, int>
{
public:
  AppKit(TaskScheduler &scheduler)
      : mpScheduler(&scheduler), mNetworkDevice(D2, LOW, D7)
  {
    _initTasks();
    _initSubscribers();
  }

  unLisp &getInterpreter()
  {
    return unLisp::getInstance();
  }

  MQTTKit &getMQTT()
  {
    return mMQTT;
  }

  NetworkDevice &getNetworkDevice()
  {
    return mNetworkDevice;
  }

  void attach()
  {
    mTaskNetwork->attach(1);
    mNetworkDevice.attach();
  }

  void connect(Broker<int, int> *broker)
  {
    broker->connect(&mNetworkDevice);
    broker->connect(&unLisp::getInstance());
    broker->connect(mpSubscriberNetwork->subscribe(NetworkScheduler::CONNECTION));
    broker->connect(mpSubscriberLisp->subscribe(unLisp::OUTPUT_BUF));
  }

  void disconnect(Broker<int, int> *broker)
  {
    broker->disconnect(&mNetworkDevice);
    broker->disconnect(&unLisp::getInstance());
    broker->disconnect(mpSubscriberNetwork->unsubscribe(NetworkScheduler::CONNECTION));
    broker->disconnect(mpSubscriberLisp->unsubscribe(unLisp::OUTPUT_BUF));
  }

private:
  void _initTasks() {
    mpScheduler->push(mTaskNetwork = TaskScheduler::make(&mNetworkDevice));
    mpScheduler->push(mTaskMQTT = TaskScheduler::make(&mMQTT));
    mpScheduler->push(unLisp::getInstance().getTask());
  }

  void _initSubscribers()
  {
    mpSubscriberNetwork = std::unique_ptr<Subscriber<int, int>>(new CallbackSubscriber<int, int>([&](int topic, int msg) {
      if (NetworkScheduler::CONNECTION == topic)
      {
        switch (msg)
        {
        case NetworkScheduler::SUCCESS:
          Serial.print("AppKit Subscriber, ip: ");
          Serial.println(WiFi.localIP());
          mTaskMQTT->attach(10);
          break;

        case NetworkScheduler::CONNECTING:
          mTaskMQTT->detach();
          break;

        case NetworkScheduler::DISCONNECTED:
          mTaskMQTT->detach();
          break;

        case NetworkScheduler::FAILED:
        default:
          mTaskMQTT->detach();
          break;
        }
      }
    }));
    mpSubscriberLisp = std::unique_ptr<Subscriber<int, int>>(new CallbackSubscriber<int, int>([&](int topic, int msg) {
      auto size = unLisp::getInstance().sizeOutput();
      auto result = unLisp::getInstance().popOutput();
      if (msg == unLisp::ADDED)
      {
        Serial.println(":) ADDED: " + String(size) + " : " + result);
      }
      else
      {
        Serial.println(":) REPLACED: " + String(size) + " : " + result);
      }
    }));
  }

  MQTTKit mMQTT;
  TaskScheduler *mpScheduler;
  NetworkDevice mNetworkDevice;

  TaskScheduler::TaskPtr mTaskHandleBroker;
  TaskScheduler::TaskPtr mTaskNetwork;
  TaskScheduler::TaskPtr mTaskMQTT;

  std::unique_ptr<Subscriber<int, int>> mpSubscriberNetwork;
  std::unique_ptr<Subscriber<int, int>> mpSubscriberLisp;
};
} // namespace uniot
