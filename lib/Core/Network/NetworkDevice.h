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

#include <Button.h>
#include <CallbackEventListener.h>
#include <EventBus.h>
#include <IEventBusConnectionKit.h>
#include <ISchedulerConnectionKit.h>
#include <NetworkScheduler.h>

namespace uniot {
class NetworkDevice : public ICoreEventBusConnectionKit, public ISchedulerConnectionKit {
 public:
  enum Topic { NETWORK_LED = FOURCC(nled) };

  NetworkDevice(Credentials &credentials, uint8_t pinBtn, uint8_t activeLevelBtn, uint8_t pinLed)
      : mNetwork(credentials, new SerialLightPrinter()),
        mNetworkLastState(NetworkScheduler::SUCCESS),
        mClickCounter(0),
        mPinLed(pinLed),
        mConfigBtn(pinBtn, activeLevelBtn, 30, [&](Button *btn, Button::Event event) {
          switch (event) {
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
        }) {
    _initSubscribers();
    _initTasks();
  }

  NetworkScheduler &getNetworkScheduler() {
    return mNetwork;
  }

  void registerWithBus(CoreEventBus &eventBus) {
    eventBus.registerEntity(&mNetwork);
    eventBus.registerEntity(mpNetworkEventListener->listenToEvent(NetworkScheduler::CONNECTION));
  }

  void unregisterFromBus(CoreEventBus &eventBus) {
    eventBus.unregisterEntity(&mNetwork);
    eventBus.unregisterEntity(mpNetworkEventListener->stopListeningToEvent(NetworkScheduler::CONNECTION));
  }

  virtual void pushTo(TaskScheduler &scheduler) override {
    scheduler.push(mNetwork);
    scheduler.push("signal_led", mpTaskSignalLed);
    scheduler.push("btn_config", mpTaskConfigBtn);
    scheduler.push("rst_click_count", mpTaskResetClickCounter);
  }

  virtual void attach() override {
    mpTaskConfigBtn->attach(100);
    mNetwork.attach();
    statusBusy();
  }

  void statusWaiting() {
    mpTaskSignalLed->attach(1000);
  }

  void statusBusy() {
    mpTaskSignalLed->attach(500);
  }

  void statusAlarm() {
    mpTaskSignalLed->attach(200);
  }

  void statusIdle() {
    mpTaskSignalLed->attach(200, 1);
  }

 private:
  void _initTasks() {
    mpTaskSignalLed = TaskScheduler::make([&](SchedulerTask &self, short t) {
      static bool pinLevel = true;
      digitalWrite(mPinLed, pinLevel = (!pinLevel && t));
      mNetwork.emitEvent(Topic::NETWORK_LED, pinLevel);
    });
    mpTaskConfigBtn = TaskScheduler::make(mConfigBtn);
    mpTaskResetClickCounter = TaskScheduler::make([&](SchedulerTask &self, short t) {
      UNIOT_LOG_DEBUG("ClickCounter = %d", mClickCounter);
      mClickCounter = 0;
    });
  }

  int _resetNetworkLastState(int newState) {
    auto oldState = mNetworkLastState;
    mNetworkLastState = newState;
    return oldState;
  }

  void _initSubscribers() {
    mpNetworkEventListener = std::unique_ptr<CoreEventListener>(new CoreCallbackEventListener([&](int topic, int msg) {
      if (NetworkScheduler::CONNECTION == topic) {
        int lastState = _resetNetworkLastState(msg);
        switch (msg) {
          case NetworkScheduler::ACCESS_POINT:
            UNIOT_LOG_DEBUG("NetworkDevice Subscriber, ACCESS_POINT");
            if (lastState != NetworkScheduler::FAILED)
              statusWaiting();
            break;

          case NetworkScheduler::SUCCESS:
            UNIOT_LOG_DEBUG("NetworkDevice Subscriber, SUCCESS, ip: %s", WiFi.localIP().toString().c_str());
            statusIdle();
            break;

          case NetworkScheduler::CONNECTING:
            UNIOT_LOG_DEBUG("NetworkDevice Subscriber, CONNECTING");
            statusBusy();
            break;

          case NetworkScheduler::DISCONNECTED:
            UNIOT_LOG_DEBUG("NetworkDevice Subscriber, DISCONNECTED");
            mNetwork.reconnect();
            break;

          case NetworkScheduler::FAILED:
            UNIOT_LOG_DEBUG("NetworkDevice Subscriber, FAILED");
          default:
            statusAlarm();
            break;
        }
      }
    }));
  }

  NetworkScheduler mNetwork;
  int mNetworkLastState;

  uint8_t mClickCounter;
  uint8_t mPinLed;
  Button mConfigBtn;

  TaskScheduler::TaskPtr mpTaskSignalLed;
  TaskScheduler::TaskPtr mpTaskConfigBtn;
  TaskScheduler::TaskPtr mpTaskResetClickCounter;

  UniquePointer<CoreEventListener> mpNetworkEventListener;
};
}  // namespace uniot
