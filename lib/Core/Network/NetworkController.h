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
#include <CBORStorage.h>
#include <EventListener.h>
#include <ISchedulerConnectionKit.h>
#include <NetworkScheduler.h>

namespace uniot {
class NetworkController : public ISchedulerConnectionKit, public CoreEventListener, public CBORStorage {
 public:
  enum Topic { NETWORK_LED = FOURCC(nled) };

  NetworkController(NetworkScheduler &network,
                    uint8_t pinBtn = UINT8_MAX,
                    uint8_t activeLevelBtn = LOW,
                    uint8_t pinLed = UINT8_MAX,
                    uint8_t activeLevelLed = HIGH,
                    uint8_t maxRebootCount = 3,
                    uint32_t rebootWindowMs = 10000)
      : CBORStorage("ctrl.cbor"),
        mpNetwork(&network),
        mNetworkLastState(NetworkScheduler::SUCCESS),
        mClickCounter(0),
        mPinLed(pinLed),
        mActiveLevelLed(activeLevelLed),
        mMaxRebootCount(maxRebootCount),
        mRebootWindowMs(rebootWindowMs),
        mRebootCount(0) {
    if (pinLed != UINT8_MAX) {
      pinMode(mPinLed, OUTPUT);
    }
    if (pinBtn != UINT8_MAX) {
      mpConfigBtn = MakeUnique<Button>(pinBtn, activeLevelBtn, 30, [&](Button *btn, Button::Event event) {
        switch (event) {
          case Button::LONG_PRESS:
            if (mClickCounter > 3)
              mpNetwork->forget();
            else
              mpNetwork->reconnect();
            break;
          case Button::CLICK:
            if (!mClickCounter) {
              mpTaskResetClickCounter->attach(5000, 1);
            }
            mClickCounter++;
          default:
            break;
        }
      });
    }
    _initTasks();
    _checkAndHandleReboot();
    CoreEventListener::listenToEvent(NetworkScheduler::Topic::CONNECTION);
  }

  virtual ~NetworkController() {
    CoreEventListener::stopListeningToEvent(NetworkScheduler::Topic::CONNECTION);
    mpNetwork = nullptr;
  }

  virtual bool store() override {
    object().put("reset", mRebootCount);
    return CBORStorage::store();
  }

  virtual bool restore() override {
    if (CBORStorage::restore()) {
      mRebootCount = object().getInt("reset");
      return true;
    }
    return false;
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
        case NetworkScheduler::AVAILABLE:
          mpNetwork->reconnect();
          break;
        case NetworkScheduler::FAILED:
          statusAlarm();
          mpNetwork->config();
          break;
        default:
          break;
      }
    }
  }

  virtual void pushTo(TaskScheduler &scheduler) override {
    scheduler.push("signal_led", mpTaskSignalLed);
    scheduler.push("rst_reboot_count", mpTaskResetRebootCounter);
    if (_hasButton()) {
      scheduler.push("btn_config", mpTaskConfigBtn);
      scheduler.push("rst_click_count", mpTaskResetClickCounter);
    }
  }

  virtual void attach() override {
    mpTaskResetRebootCounter->once(mRebootWindowMs);
    if (_hasButton()) {
      mpTaskConfigBtn->attach(100);
    }
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

  Button *getButton() {
    return _hasButton() ? mpConfigBtn.get() : nullptr;
  }

 private:
  void _initTasks() {
    mpTaskSignalLed = TaskScheduler::make([&](SchedulerTask &self, short t) {
      static bool signalLevel = true;
      signalLevel = (!signalLevel && t);
      CoreEventListener::emitEvent(Topic::NETWORK_LED, signalLevel);

      if (_hasLed()) {
        digitalWrite(mPinLed, signalLevel ? mActiveLevelLed : !mActiveLevelLed);
      }
    });

    if (_hasButton()) {
      mpTaskConfigBtn = TaskScheduler::make(*mpConfigBtn);
      mpTaskResetClickCounter = TaskScheduler::make([&](SchedulerTask &self, short t) {
        UNIOT_LOG_DEBUG("ClickCounter = %d", mClickCounter);
        mClickCounter = 0;
      });
    }

    mpTaskResetRebootCounter = TaskScheduler::make([&](SchedulerTask &self, short t) {
      mRebootCount = 0;
      NetworkController::store();
    });
  }

  void _checkAndHandleReboot() {
    NetworkController::restore();
    mRebootCount++;
    if (mRebootCount >= mMaxRebootCount) {
      mpTaskResetRebootCounter->detach();
      mpNetwork->forget();
      mRebootCount = 0;
    }
    NetworkController::store();
  }

  int _resetNetworkLastState(int newState) {
    auto oldState = mNetworkLastState;
    mNetworkLastState = newState;
    return oldState;
  }

  inline bool _hasButton() {
    return mpConfigBtn.get() != nullptr;
  }

  inline bool _hasLed() {
    return mPinLed != UINT8_MAX;
  }

  NetworkScheduler *mpNetwork;
  int mNetworkLastState;

  uint8_t mClickCounter;
  uint8_t mPinLed;
  uint8_t mActiveLevelLed;
  uint8_t mMaxRebootCount;
  uint32_t mRebootWindowMs;
  uint8_t mRebootCount;

  UniquePointer<Button> mpConfigBtn;

  TaskScheduler::TaskPtr mpTaskSignalLed;
  TaskScheduler::TaskPtr mpTaskConfigBtn;
  TaskScheduler::TaskPtr mpTaskResetClickCounter;
  TaskScheduler::TaskPtr mpTaskResetRebootCounter;
};
}  // namespace uniot
