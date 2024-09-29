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
#include <EventListener.h>
#include <ISchedulerConnectionKit.h>
#include <NetworkScheduler.h>

namespace uniot {
class NetworkController : public ISchedulerConnectionKit, public CoreEventListener {
 public:
  enum Topic { NETWORK_LED = FOURCC(nled) };

  NetworkController(NetworkScheduler &network, uint8_t pinBtn, uint8_t activeLevelBtn, uint8_t pinLed)
      : mpNetwork(&network),
        mNetworkLastState(NetworkScheduler::SUCCESS),
        mClickCounter(0),
        mPinLed(pinLed),
        mConfigBtn(pinBtn, activeLevelBtn, 30, [&](Button *btn, Button::Event event) {
          switch (event) {
            case Button::LONG_PRESS:
              if (mClickCounter > 3)
                mpNetwork->forget();
              else
                mpNetwork->reconnect();
              break;
            case Button::CLICK:
              if (!mClickCounter)
                mpTaskResetClickCounter->attach(5000, 1);
              mClickCounter++;
            default:
              break;
          }
        }) {
    pinMode(mPinLed, OUTPUT);
    _initTasks();
    CoreEventListener::listenToEvent(NetworkScheduler::Topic::CONNECTION);
  }

  virtual ~NetworkController() {
    CoreEventListener::stopListeningToEvent(NetworkScheduler::Topic::CONNECTION);
    mpNetwork = nullptr;
  }

  virtual void onEventReceived(unsigned int topic, int msg) override {
    if (NetworkScheduler::CONNECTION == topic) {
      int lastState = _resetNetworkLastState(msg);
      switch (msg) {
        case NetworkScheduler::ACCESS_POINT:
          if (lastState != NetworkScheduler::FAILED) {
            statusWaiting();
          }
          break;
        case NetworkScheduler::SUCCESS:
          statusIdle();
          break;
        case NetworkScheduler::CONNECTING:
          statusBusy();
          break;
        case NetworkScheduler::DISCONNECTED:
          if (lastState != NetworkScheduler::CONNECTING) {
            // If previous state was CONNECTING, most likely there was a manual reconnect request
            mpNetwork->reconnect();
          }
          break;
        case NetworkScheduler::FAILED:
        default:
          statusAlarm();
          break;
      }
    }
  }

  virtual void pushTo(TaskScheduler &scheduler) override {
    scheduler.push("signal_led", mpTaskSignalLed);
    scheduler.push("btn_config", mpTaskConfigBtn);
    scheduler.push("rst_click_count", mpTaskResetClickCounter);
  }

  virtual void attach() override {
    mpTaskConfigBtn->attach(100);
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

  Button &getButton() {
    return mConfigBtn;
  }

 private:
  void _initTasks() {
    mpTaskSignalLed = TaskScheduler::make([&](SchedulerTask &self, short t) {
      static bool pinLevel = true;
      digitalWrite(mPinLed, pinLevel = (!pinLevel && t));
      CoreEventListener::emitEvent(Topic::NETWORK_LED, pinLevel);
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

  NetworkScheduler *mpNetwork;
  int mNetworkLastState;

  uint8_t mClickCounter;
  uint8_t mPinLed;
  Button mConfigBtn;

  TaskScheduler::TaskPtr mpTaskSignalLed;
  TaskScheduler::TaskPtr mpTaskConfigBtn;
  TaskScheduler::TaskPtr mpTaskResetClickCounter;
};
}  // namespace uniot
