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

#include <CallbackEventListener.h>
#include <CrashStorage.h>
#include <Date.h>
#include <EventBus.h>
#include <IEventBusConnectionKit.h>
#include <ISchedulerConnectionKit.h>
#include <LispDevice.h>
#include <LispPrimitives.h>
#include <Logger.h>
#include <MQTTKit.h>
#include <NetworkDevice.h>
#include <PinMap.h>
#include <unLisp.h>

namespace uniot {
class AppKit : public ICoreEventBusConnectionKit, public ISchedulerConnectionKit {
 public:
  AppKit(AppKit const &) = delete;
  void operator=(AppKit const &) = delete;

  static AppKit &getInstance(uint8_t pinBtn, uint8_t activeLevelBtn, uint8_t pinLed) {
    static AppKit instance(pinBtn, activeLevelBtn, pinLed);
    return instance;
  }

  unLisp &getLisp() {
    return unLisp::getInstance();
  }

  MQTTKit &getMQTT() {
    return mMQTT;
  }

  const Credentials &getCredentials() {
    return mCredentials;
  }

  void begin() {
    analogWriteRange(1023);
    attach();
    mNetworkDevice.begin();
    mLispDevice.runStoredCode();
  }

  void pushTo(TaskScheduler *scheduler) override {
    scheduler->push(&mNetworkDevice);
    scheduler->push(mTaskMQTT);
    scheduler->push(mTaskLispButton);
    scheduler->push(getLisp().getTask());
  }

  void attach() override {
    mNetworkDevice.attach();
    mTaskLispButton->attach(100);
  }

  void registerWithBus(CoreEventBus *eventBus) override {
    eventBus->openDataChannel(unLisp::Channel::OUT_LISP, 10);
    eventBus->openDataChannel(unLisp::Channel::OUT_LISP_ERR, 1);
    eventBus->openDataChannel(unLisp::Channel::OUT_EVENT, 10);
    eventBus->openDataChannel(unLisp::Channel::IN_EVENT, 20);
    eventBus->registerKit(&mNetworkDevice);
    eventBus->registerEntity(&getLisp());
    eventBus->registerEntity(&mLispDevice);
    eventBus->registerEntity(mpNetworkEventListener->listenToEvent(NetworkScheduler::CONNECTION));
    eventBus->registerEntity(mpLispEventListener->listenToEvent(unLisp::Topic::OUT_LISP_EVENT));
  }

  void unregisterFromBus(CoreEventBus *eventBus) override {
    eventBus->closeDataChannel(unLisp::Channel::OUT_LISP);
    eventBus->closeDataChannel(unLisp::Channel::OUT_LISP_ERR);
    eventBus->closeDataChannel(unLisp::Channel::OUT_EVENT);
    eventBus->closeDataChannel(unLisp::Channel::IN_EVENT);
    eventBus->unregisterKit(&mNetworkDevice);
    eventBus->unregisterEntity(&getLisp());
    eventBus->unregisterEntity(&mLispDevice);
    eventBus->unregisterEntity(mpNetworkEventListener->stopListeningToEvent(NetworkScheduler::CONNECTION));
    eventBus->unregisterEntity(mpLispEventListener->stopListeningToEvent(unLisp::Topic::OUT_LISP_EVENT));
  }

 private:
  AppKit(uint8_t pinBtn, uint8_t activeLevelBtn, uint8_t pinLed)
      : mMQTT(mCredentials, [this](CBORObject &info) {
          auto arr = info.putArray("primitives");
          getLisp().serializeNamesOfPrimitives(arr.get());
          arr->closeArray();

          // TODO: add uniot core version
          info.put("timestamp", (long)Date::now());
          info.put("creator", mCredentials.getCreatorId().c_str());
          info.put("public_key", mCredentials.getPublicKey().c_str());
          info.put("mqtt_size", MQTT_MAX_PACKET_SIZE);
          info.put("debug", UNIOT_LOG_ENABLED);
          info.put("d_in", UniotPinMap.getDigitalInputLength());
          info.put("d_out", UniotPinMap.getDigitalOutputLength());
          info.put("a_in", UniotPinMap.getAnalogInputLength());
          info.put("a_out", UniotPinMap.getAnalogOutputLength());
        }),
        mLispButton(pinBtn, activeLevelBtn, 30),
        mNetworkDevice(mCredentials, pinBtn, activeLevelBtn, pinLed) {
    _initMqtt();
    _initTasks();
    _initSubscribers();
    _initPrimitives();
  }

  void _initMqtt() {
    // TODO: should I move configs to the Credentials class?
    mMQTT.setServer("mqtt.uniot.io", 1883);
    mMQTT.addDevice(&mLispDevice);
    mLispDevice.syncSubscriptions();
  }

  void _initTasks() {
    mTaskMQTT = TaskScheduler::make(&mMQTT);
    mTaskLispButton = TaskScheduler::make(mLispButton.getTaskCallback());
  }

  void _initPrimitives() {
    getLisp().pushPrimitive(globalPrimitive(dwrite));
    getLisp().pushPrimitive(globalPrimitive(dread));
    getLisp().pushPrimitive(globalPrimitive(awrite));
    getLisp().pushPrimitive(globalPrimitive(aread));
    getLisp().pushPrimitive(globalPrimitive(bclicked));

    PrimitiveExpeditor::getGlobalRegister().link("bclicked", &mLispButton);
  }

  void _initSubscribers() {
    mpNetworkEventListener = std::unique_ptr<CoreEventListener>(new CoreCallbackEventListener([&](int topic, int msg) {
      if (NetworkScheduler::CONNECTION == topic) {
        switch (msg) {
          case NetworkScheduler::SUCCESS:
            Serial.print("AppKit Subscriber, ip: ");
            Serial.println(WiFi.localIP());
            mTaskMQTT->attach(10);
            mMQTT.renewSubscriptions();
            break;
          case NetworkScheduler::ACCESS_POINT:
            Serial.println("AppKit Subscriber, ACCESS_POINT ");
            mTaskMQTT->detach();
            break;

          case NetworkScheduler::CONNECTING:
            Serial.println("AppKit Subscriber, CONNECTING ");
            mTaskMQTT->detach();
            break;

          case NetworkScheduler::DISCONNECTED:
            Serial.println("AppKit Subscriber, DISCONNECTED ");
            mTaskMQTT->detach();
            break;

          case NetworkScheduler::FAILED:
          default:
            Serial.println("AppKit Subscriber, FAILED ");
            mTaskMQTT->detach();
            break;
        }
      }
    }));

    mpLispEventListener = std::unique_ptr<CoreEventListener>(new CoreCallbackEventListener([&](int topic, int msg) {
      if (msg == unLisp::Msg::OUT_NEW_EVENT) {
        mpLispEventListener->receiveDataFromChannel(unLisp::Channel::OUT_EVENT, [this](unsigned int id, bool empty, Bytes data) {
          if (!empty) {
            auto event = CBORObject(data);
            event.put("timestamp", (long)Date::now())
                .putMap("sender")
                .put("type", "device")
                .put("id", mCredentials.getDeviceId().c_str());
            auto eventData = event.build();
            auto eventID = event.getString("eventID");
            mLispDevice.publishGroup("all", "event/" + eventID, eventData);
          }
        });
      }
    }));
  }

  Credentials mCredentials;
  MQTTKit mMQTT;
  LispDevice mLispDevice;
  Button mLispButton;

  NetworkDevice mNetworkDevice;

  TaskScheduler::TaskPtr mTaskMQTT;
  TaskScheduler::TaskPtr mTaskLispButton;

  UniquePointer<CoreEventListener> mpNetworkEventListener;
  UniquePointer<CoreEventListener> mpLispEventListener;
};
}  // namespace uniot
