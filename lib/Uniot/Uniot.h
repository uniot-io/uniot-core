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
 public:
  using TimerId = uint32_t;
  using ListenerId = uint32_t;
  static constexpr TimerId INVALID_TIMER_ID = 0;
  static constexpr ListenerId INVALID_LISTENER_ID = 0;

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

  template <typename... Pins>
  void registerLispDigitalOutput(uint8_t first, Pins... pins) {
    uniot::PrimitiveExpeditor::getRegisterManager().setDigitalOutput(first, pins...);
  }

  template <typename... Pins>
  void registerLispDigitalInput(uint8_t first, Pins... pins) {
    uniot::PrimitiveExpeditor::getRegisterManager().setDigitalInput(first, pins...);
  }

  template <typename... Pins>
  void registerLispAnalogInput(uint8_t first, Pins... pins) {
    uniot::PrimitiveExpeditor::getRegisterManager().setAnalogInput(first, pins...);
  }

  template <typename... Pins>
  void registerLispAnalogOutput(uint8_t first, Pins... pins) {
    uniot::PrimitiveExpeditor::getRegisterManager().setAnalogOutput(first, pins...);
  }

  bool registerLispButton(uniot::Button* button, uint32_t id = FOURCC(_btn)) {
    return uniot::PrimitiveExpeditor::getRegisterManager().link(uniot::primitive::name::bclicked, button, id);
  }

  bool registerLispObject(const String& primitiveName, uniot::RecordPtr link, uint32_t id = FOURCC(____)) {
    return uniot::PrimitiveExpeditor::getRegisterManager().link(primitiveName, link, id);
  }

  TimerId setInterval(std::function<void()> callback, uint32_t intervalMs, short times = 0) {
    if (!callback) {
      return INVALID_TIMER_ID;
    }

    auto id = _generateTimerId();
    auto task = uniot::TaskScheduler::make([this, id, callback = std::move(callback)](uniot::SchedulerTask&, short remainingTimes) {
      callback();

      if (!remainingTimes) {
        mActiveTimers.remove(id);
      }
    });

    mActiveTimers.put(id, task);
    mScheduler.push(nullptr, task);
    task->attach(intervalMs, times);
    return id;
  }

  TimerId setTimeout(std::function<void()> callback, uint32_t timeoutMs) {
    return setInterval(std::move(callback), timeoutMs, 1);
  }

  TimerId setImmediate(std::function<void()> callback) {
    return setTimeout(std::move(callback), 1);
  }

  bool cancelTimer(TimerId id) {
    if (id == INVALID_TIMER_ID) {
      return false;
    }

    auto task = mActiveTimers.get(id, nullptr);
    if (task) {
      task->detach();
      return mActiveTimers.remove(id);
    }
    return false;
  }

  bool isTimerActive(TimerId id) {
    if (id == INVALID_TIMER_ID) {
      return false;
    }

    auto task = mActiveTimers.get(id, nullptr);
    return task && task->isAttached();
  }

  int getActiveTimersCount() const {
    return mActiveTimers.calcSize();
  }

  uniot::TaskScheduler::TaskPtr createTask(const char* name, uniot::SchedulerTask::SchedulerTaskCallback callback) {
    auto task = uniot::TaskScheduler::make(std::move(callback));
    mScheduler.push(name, task);
    return task;
  }

  template <typename... Topics>
  ListenerId addSystemListener(std::function<void(unsigned int, int)> callback, unsigned int firstTopic, Topics... otherTopics) {
    if (!callback) {
      return INVALID_LISTENER_ID;
    }

    unsigned int topics[] = {firstTopic, static_cast<unsigned int>(otherTopics)...};
    size_t count = sizeof...(otherTopics) + 1;

    auto listener = uniot::MakeShared<uniot::CoreCallbackEventListener>(callback);
    for (size_t i = 0; i < count; ++i) {
      listener->listenToEvent(topics[i]);
    }

    if (mEventBus.registerEntity(listener.get())) {
      auto id = _generateListenerId();
      mActiveListeners.put(id, listener);
      return id;
    }

    return INVALID_LISTENER_ID;
  }

  bool removeSystemListener(ListenerId id) {
    if (id == INVALID_LISTENER_ID) {
      return false;
    }

    auto listener = mActiveListeners.get(id, nullptr);
    if (listener) {
      mEventBus.unregisterEntity(listener.get());
      return mActiveListeners.remove(id);
    }
    return false;
  }

  template <typename... Topics>
  size_t removeSystemListeners(unsigned int firstTopic, Topics... otherTopics) {
    unsigned int topics[] = {firstTopic, static_cast<unsigned int>(otherTopics)...};
    size_t topicCount = sizeof...(otherTopics) + 1;
    size_t removedCount = 0;

    mActiveListeners.begin();
    while (!mActiveListeners.isEnd()) {
      auto listener = mActiveListeners.current().second;
      bool shouldRemove = false;

      if (listener) {
        for (size_t i = 0; i < topicCount; ++i) {
          if (listener->isListeningToEvent(topics[i])) {
            shouldRemove = true;
            break;
          }
        }
      }

      if (shouldRemove) {
        mEventBus.unregisterEntity(listener.get());
        mActiveListeners.deleteCurrent();
        removedCount++;
      } else {
        mActiveListeners.next();
      }
    }

    return removedCount;
  }

  bool isSystemListenerActive(ListenerId id) {
    if (id == INVALID_LISTENER_ID) {
      return false;
    }

    return mActiveListeners.exist(id);
  }

  int getActiveListenersCount() const {
    return mActiveListeners.calcSize();
  }

  void emitSystemEvent(unsigned int topic, int message) {
    mEventBus.emitEvent(topic, message);
  }

  ListenerId addWifiStatusLedListener(std::function<void(bool)> callback) {
    if (!callback) {
      return INVALID_LISTENER_ID;
    }

    return addSystemListener(
      [callback = std::move(callback)](unsigned int topic, int message) {
        if (topic == uniot::NetworkController::WIFI_STATUS_LED) {
          callback(message != 0);
        }
      },
      uniot::NetworkController::WIFI_STATUS_LED);
  }

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
      mpNetworkControllerConfig = uniot::MakeUnique<uniot::AppKit::NetworkControllerConfig>();
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
      mpNetworkControllerConfig.reset(nullptr);
    }
  }

  TimerId _generateTimerId() {
    static TimerId nextId = 1;
    return nextId++;
  }

  ListenerId _generateListenerId() {
    static ListenerId nextId = 1;
    return nextId++;
  }

  uniot::TaskScheduler mScheduler;
  uniot::CoreEventBus mEventBus;
  uniot::Map<TimerId, uniot::TaskScheduler::TaskPtr> mActiveTimers;
  uniot::Map<ListenerId, uniot::SharedPointer<uniot::CoreCallbackEventListener>> mActiveListeners;
  uniot::UniquePointer<uniot::AppKit::NetworkControllerConfig> mpNetworkControllerConfig;
};

extern UniotCore Uniot;
