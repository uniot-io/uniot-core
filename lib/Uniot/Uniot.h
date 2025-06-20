/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2024 Uniot <contact@uniot.io>
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

#include <AppKit.h>
#include <Arduino.h>
#include <Credentials.h>
#include <Date.h>
#include <EventBus.h>
#include <Logger.h>
#include <TaskScheduler.h>

class UniotCore {
 protected:
  class Timer {
   public:
    Timer() : mTask(nullptr) {}
    Timer(uniot::TaskScheduler::TaskPtr task) : mTask(task) {}

    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

    Timer(Timer&& other) noexcept : mTask(std::move(other.mTask)) {}
    Timer& operator=(Timer&& other) noexcept {
      if (this != &other) {
        mTask = std::move(other.mTask);
      }
      return *this;
    }

    void cancel() {
      if (mTask) {
        mTask->detach();
        mTask.reset();
      }
    }

    bool isActive() const {
      return mTask && mTask->isAttached();
    }

   private:
    uniot::TaskScheduler::TaskPtr mTask;
  };

 public:
  UniotCore() : mScheduler(), mEventBus(FOURCC(main)), mpNetworkControllerConfig(nullptr) {}

  void configWiFiResetButton(uint8_t pinBtn, uint8_t activeLevelBtn = LOW, bool registerLispBtn = true) {
    _createNetworkControllerConfig();
    mpNetworkControllerConfig->pinBtn = pinBtn;
    mpNetworkControllerConfig->activeLevelBtn = activeLevelBtn;
    mpNetworkControllerConfig->registerLispBtn = registerLispBtn;
  }

  void configWiFiStatusLed(uint8_t pinLed, uint8_t activeLevelLed = HIGH) {
    _createNetworkControllerConfig();
    mpNetworkControllerConfig->pinLed = pinLed;
    mpNetworkControllerConfig->activeLevelLed = activeLevelLed;
  }

  void configWiFiResetOnReboot(uint8_t maxRebootCount, uint32_t rebootWindowMs = 10000) {
    _createNetworkControllerConfig();
    mpNetworkControllerConfig->maxRebootCount = maxRebootCount;
    mpNetworkControllerConfig->rebootWindowMs = rebootWindowMs;
  }

  void configWiFiCredentials(const String& ssid, const String& password = "") {
    _createNetworkControllerConfig();
    auto success = getAppKit().setWiFiCredentials(ssid, password);
    UNIOT_LOG_ERROR_IF(!success, "Failed to set WiFi credentials");
  }

  void enablePeriodicDateSave(uint32_t periodSeconds = 5 * 60) {
    if (periodSeconds > 0) {
      auto taskStoreDate = uniot::TaskScheduler::make(uniot::Date::getInstance());
      mScheduler.push("store_date", taskStoreDate);
      taskStoreDate->attach(periodSeconds * 1000L);
    }
  }

  void addLispPrimitive(Primitive* primitive) {
    getAppKit().getLisp().pushPrimitive(primitive);
  }

  void setLispEventInterceptor(uniot::LispEventInterceptor interceptor) {
    getAppKit().setLispEventInterceptor(interceptor);
  }

  void publishLispEvent(const String& eventID, int32_t value) {
    getAppKit().publishLispEvent(eventID, value);
  }

  Timer setInterval(std::function<void()> callback, uint32_t intervalMs, short times = 0) {
    if (!callback) {
      return Timer();
    }

    auto task = uniot::TaskScheduler::make([callback = std::move(callback)](uniot::SchedulerTask&, short) {
      callback();
    });
    mScheduler.push(nullptr, task);
    task->attach(intervalMs, times);
    // Return relies on RVO (Return Value Optimization) to construct Timer directly
    // at the call site, avoiding any move/copy operations even if they're available
    return Timer(task);
  }

  Timer setTimeout(std::function<void()> callback, uint32_t timeoutMs) {
    return setInterval(std::move(callback), timeoutMs, 1);
  }

  Timer setImmediate(std::function<void()> callback) {
    return setTimeout(std::move(callback), 1);
  }

  // todo: LED, network, mqtt callbacks

  void begin(uint32_t eventBusTaskPeriod = 10) {
    UNIOT_LOG_SET_READY();

    auto& app = uniot::AppKit::getInstance();

    if (mpNetworkControllerConfig) {
      app.configureNetworkController(*mpNetworkControllerConfig);
      _deleteNetworkControllerConfig();
    }

    mEventBus.registerKit(app);

    auto taskHandleEventBus = uniot::TaskScheduler::make(mEventBus);
    mScheduler.push("event_bus", taskHandleEventBus);
    taskHandleEventBus->attach(eventBusTaskPeriod);

    mScheduler.push(app);
    app.attach();
  }

  void loop() {
    mScheduler.loop();
  }

  uniot::AppKit& getAppKit() {
    return uniot::AppKit::getInstance();
  }

  uniot::CoreEventBus& getEventBus() {
    return mEventBus;
  }

  uniot::TaskScheduler& getScheduler() {
    return mScheduler;
  }

 private:
  void _createNetworkControllerConfig() {
    if (!mpNetworkControllerConfig) {
      mpNetworkControllerConfig = new uniot::AppKit::NetworkControllerConfig();
      mpNetworkControllerConfig->pinBtn = UINT8_MAX;  // Not used by default
      mpNetworkControllerConfig->activeLevelBtn = LOW;
      mpNetworkControllerConfig->pinLed = UINT8_MAX;  // Not used by default
      mpNetworkControllerConfig->activeLevelLed = HIGH;
      mpNetworkControllerConfig->maxRebootCount = 5;
      mpNetworkControllerConfig->rebootWindowMs = 10000;
      mpNetworkControllerConfig->registerLispBtn = true;
    }
  }

  void _deleteNetworkControllerConfig() {
    if (mpNetworkControllerConfig) {
      delete mpNetworkControllerConfig;
      mpNetworkControllerConfig = nullptr;
    }
  }

  uniot::TaskScheduler mScheduler;
  uniot::CoreEventBus mEventBus;
  uniot::AppKit::NetworkControllerConfig* mpNetworkControllerConfig;
};

extern UniotCore Uniot;
