/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2023 Uniot <contact@uniot.io>
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

#include <CBORStorage.h>
#include <CallbackEventListener.h>
#include <EventBus.h>
#include <IEventBusKitConnection.h>
#include <ISchedulerKitConnection.h>
#include <NTPClient.h>
#include <NetworkDevice.h>
#include <WiFiUdp.h>

namespace uniot {
class Date : public ICoreEventBusKitConnection, public ISchedulerKitConnection, public CBORStorage {
 public:
  Date() : CBORStorage("date.cbor"), mNTPClient(mWiFiUDP) {
    _restoreEpoch();
    _initTasks();
    _initSubscribers();
  }

  unsigned long now() {
    return mNTPClient.getEpochTime();
  }

  String getFormattedTime() {
    return mNTPClient.getFormattedTime();
  }

  void begin() {
    attach();
  }

  void pushTo(TaskScheduler *scheduler) override {
    scheduler->push(mTaskPrintTime);
    scheduler->push(mTaskNTPUpdate);
  }

  void attach() override {
    mTaskPrintTime->attach(500);
  }

  bool store() override {
    object().put("epoch", mNTPClient.getEpochTime());
    return CBORStorage::store();
  }

  bool restore() override {
    if (CBORStorage::restore()) {
      unsigned int currentEpoc = object().getInt("epoch");
      mNTPClient.setEpochTime(currentEpoc);
      return true;
    }
    UNIOT_LOG_ERROR("%s", "epoch not restored");
    return false;
  }

  void connect(CoreEventBus *eventBus) override {
    eventBus->connect(mpNetworkEventListener->listenToEvent(NetworkScheduler::CONNECTION));
  }

  void disconnect(CoreEventBus *eventBus) override {
    eventBus->disconnect(mpNetworkEventListener->stopListeningToEvent(NetworkScheduler::CONNECTION));
  }

 private:
  void _restoreEpoch() {
    restore();
  }

  void _initTasks() {
    mTaskPrintTime = TaskScheduler::make([&](short t) {
      Serial.println(now());
    });
    mTaskNTPUpdate = TaskScheduler::make([&](short t) {
      if (!mNTPClient.update()) {
        UNIOT_LOG_ERROR("failed to update current epoch from NTP server");
      }
      if (!store()) {
        UNIOT_LOG_ERROR("failed to store current epoch in CBORStorage");
      }
    });
  }

  void _initSubscribers() {
    mpNetworkEventListener = std::unique_ptr<CoreEventListener>(new CoreCallbackEventListener([&](int topic, int msg) {
      if (NetworkScheduler::CONNECTION == topic) {
        switch (msg) {
          case NetworkScheduler::SUCCESS:
            Serial.print("Date Subscriber, epoch: ");
            Serial.println(mNTPClient.getEpochTime());
            mTaskNTPUpdate->attach(mNTPClient.getUpdateInterval());
            mNTPClient.begin();
            mNTPClient.forceUpdate();
            break;

          case NetworkScheduler::ACCESS_POINT:
            Serial.println("Date Subscriber, ACCESS_POINT");
            mTaskNTPUpdate->detach();
            mNTPClient.end();
            break;

          case NetworkScheduler::CONNECTING:
            Serial.println("Date Subscriber, CONNECTING");
            mTaskNTPUpdate->detach();
            mNTPClient.end();
            break;

          case NetworkScheduler::DISCONNECTED:
            Serial.println("Date Subscriber, DISCONNECTED");
            mTaskNTPUpdate->detach();
            mNTPClient.end();
            break;

          case NetworkScheduler::FAILED:
          default:
            Serial.println("Date Subscriber, FAILED");
            mTaskNTPUpdate->detach();
            mNTPClient.end();
            break;
        }
      }
    }));
  }

  WiFiUDP mWiFiUDP;
  NTPClient mNTPClient;

  TaskScheduler::TaskPtr mTaskNTPUpdate;
  TaskScheduler::TaskPtr mTaskPrintTime;

  UniquePointer<CoreEventListener> mpNetworkEventListener;
};
}  // namespace uniot
