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
#include <NetworkController.h>
#include <Singleton.h>
#include <TopDevice.h>
#include <unLisp.h>

namespace uniot {
class AppKit : public ICoreEventBusConnectionKit, public ISchedulerConnectionKit, public Singleton<AppKit> {
  friend class Singleton<AppKit>;

 public:
  struct NetworkControllerConfig {
    uint8_t pinBtn = UINT8_MAX;
    uint8_t activeLevelBtn = LOW;
    uint8_t pinLed = UINT8_MAX;
    uint8_t activeLevelLed = HIGH;
    uint8_t maxRebootCount = 3;
    uint32_t rebootWindowMs = 10000;
  };

  unLisp &getLisp() {
    return unLisp::getInstance();
  }

  MQTTKit &getMQTT() {
    return mMQTT;
  }

  const Credentials &getCredentials() {
    return mCredentials;
  }

  virtual void pushTo(TaskScheduler &scheduler) override {
    scheduler.push(mNetwork);
    scheduler.push(mMQTT);
    scheduler.push("lisp_task", getLisp().getTask());

    mTopDevice.setScheduler(scheduler);

    if (mpNetworkDevice) {
      scheduler.push(*mpNetworkDevice);
    } else {
      UNIOT_LOG_WARN("Configure Network Controller before pushing to the scheduler");
    }
  }

  virtual void attach() override {
    _initPrimitives();
    mNetwork.attach();
    mMQTT.attach();

    if (mpNetworkDevice) {
      mpNetworkDevice->attach();
    }

#if defined(ESP8266)
    analogWriteRange(1023);
#elif defined(ESP32)
    analogWriteResolution(10);
#endif
    mLispDevice.runStoredCode();
  }

  virtual void registerWithBus(CoreEventBus &eventBus) override {
    eventBus.openDataChannel(NetworkScheduler::Channel::OUT_SSID, 1);
    eventBus.openDataChannel(unLisp::Channel::OUT_LISP, 5);
    eventBus.openDataChannel(unLisp::Channel::OUT_LISP_LOG, 10);
    eventBus.openDataChannel(unLisp::Channel::OUT_LISP_ERR, 1);
    eventBus.openDataChannel(unLisp::Channel::OUT_EVENT, 10);
    eventBus.openDataChannel(unLisp::Channel::IN_EVENT, 20);
    eventBus.registerEntity(&Date::getInstance());
    eventBus.registerEntity(&mNetwork);
    eventBus.registerEntity(&mMQTT);
    eventBus.registerEntity(&getLisp());
    eventBus.registerEntity(&mLispDevice);
    eventBus.registerEntity(mpNetworkEventListener
                                ->listenToEvent(NetworkScheduler::Topic::CONNECTION)
                                ->listenToEvent(MQTTKit::Topic::CONNECTION));

    if (mpNetworkDevice) {
      eventBus.registerEntity(mpNetworkDevice.get());
    } else {
      UNIOT_LOG_WARN("Configure Network Controller before registering to the event bus");
    }
  }

  virtual void unregisterFromBus(CoreEventBus &eventBus) override {
    eventBus.closeDataChannel(NetworkScheduler::Channel::OUT_SSID);
    eventBus.closeDataChannel(unLisp::Channel::OUT_LISP);
    eventBus.closeDataChannel(unLisp::Channel::OUT_LISP_LOG);
    eventBus.closeDataChannel(unLisp::Channel::OUT_LISP_ERR);
    eventBus.closeDataChannel(unLisp::Channel::OUT_EVENT);
    eventBus.closeDataChannel(unLisp::Channel::IN_EVENT);
    eventBus.unregisterEntity(&Date::getInstance());
    eventBus.unregisterEntity(&mNetwork);
    eventBus.unregisterEntity(&mMQTT);
    eventBus.unregisterEntity(&getLisp());
    eventBus.unregisterEntity(&mLispDevice);
    eventBus.unregisterEntity(mpNetworkEventListener
                                  ->stopListeningToEvent(NetworkScheduler::Topic::CONNECTION)
                                  ->stopListeningToEvent(MQTTKit::Topic::CONNECTION));

    if (mpNetworkDevice) {
      eventBus.unregisterEntity(mpNetworkDevice.get());
    }
  }

  void configureNetworkController(const NetworkControllerConfig &config) {
    configureNetworkController(config.pinBtn,
                               config.activeLevelBtn,
                               config.pinLed,
                               config.activeLevelLed,
                               config.maxRebootCount,
                               config.rebootWindowMs);
  }

  void configureNetworkController(uint8_t pinBtn = UINT8_MAX,
                                  uint8_t activeLevelBtn = LOW,
                                  uint8_t pinLed = UINT8_MAX,
                                  uint8_t activeLevelLed = HIGH,
                                  uint8_t maxRebootCount = 3,
                                  uint32_t rebootWindowMs = 10000) {
    if (mpNetworkDevice) {
      UNIOT_LOG_WARN("Network Controller already configured");
      return;
    }

    mpNetworkDevice = MakeUnique<NetworkController>(mNetwork, pinBtn, activeLevelBtn, pinLed, activeLevelLed, maxRebootCount, rebootWindowMs);
    auto ctrlBtn = mpNetworkDevice->getButton();
    if (ctrlBtn) {
      PrimitiveExpeditor::getRegisterManager().link(primitive::name::bclicked, ctrlBtn, FOURCC(ctrl));
    }
  }

 private:
  AppKit()
      : mNetwork(mCredentials),
        mMQTT(mCredentials, [this](CBORObject &info) {
          auto obj = info.putMap("primitives");
          getLisp().serializePrimitives(obj);

          auto registers = info.putMap("misc").putMap("registers");
          PrimitiveExpeditor::getRegisterManager().serializeRegisters(registers);

          // TODO: add uniot core version
          info.put("timestamp", static_cast<int64_t>(Date::now()));
          info.put("creator", mCredentials.getCreatorId().c_str());
          info.put("public_key", mCredentials.getPublicKey().c_str());
          info.put("mqtt_size", MQTT_MAX_PACKET_SIZE);
          info.put("debug", UNIOT_LOG_ENABLED);
        }),
        mpNetworkDevice(nullptr) {
    _initMqtt();
    _initTasks();
    _initListeners();
  }

  inline void _initMqtt() {
    // TODO: should I move configs to the Credentials class?
    mMQTT.setServer("mqtt.uniot.io", 1883);
    mMQTT.addDevice(mTopDevice);
    mMQTT.addDevice(mLispDevice);
    mTopDevice.syncSubscriptions();
    mLispDevice.syncSubscriptions();
  }

  inline void _initTasks() {
    // TODO: init new tasks here
  }

  inline void _initPrimitives() {
    if (PrimitiveExpeditor::getRegisterManager().getRegisterLength(primitive::name::dwrite)) {
      getLisp().pushPrimitive(primitive::dwrite);
    }
    if (PrimitiveExpeditor::getRegisterManager().getRegisterLength(primitive::name::dread)) {
      getLisp().pushPrimitive(primitive::dread);
    }
    if (PrimitiveExpeditor::getRegisterManager().getRegisterLength(primitive::name::awrite)) {
      getLisp().pushPrimitive(primitive::awrite);
    }
    if (PrimitiveExpeditor::getRegisterManager().getRegisterLength(primitive::name::aread)) {
      getLisp().pushPrimitive(primitive::aread);
    }
    if (PrimitiveExpeditor::getRegisterManager().getRegisterLength(primitive::name::bclicked)) {
      getLisp().pushPrimitive(primitive::bclicked);
    }
  }

  inline void _initListeners() {
    mpNetworkEventListener = MakeUnique<CoreCallbackEventListener>([&](int topic, int msg) {
      if (NetworkScheduler::CONNECTION == topic) {
        switch (msg) {
          case NetworkScheduler::SUCCESS:
            UNIOT_LOG_DEBUG("AppKit Subscriber, SUCCESS, ip: %s", WiFi.localIP().toString().c_str());
            break;
          case NetworkScheduler::ACCESS_POINT:
            UNIOT_LOG_DEBUG("AppKit Subscriber, ACCESS_POINT");
            mpNetworkEventListener->receiveDataFromChannel(NetworkScheduler::Channel::OUT_SSID, [this](unsigned int id, bool empty, Bytes data) {
              if (!empty) {
                UNIOT_LOG_DEBUG("SSID: %s", data.terminate().c_str());
              }
            });
            break;

          case NetworkScheduler::CONNECTING:
            UNIOT_LOG_DEBUG("AppKit Subscriber, CONNECTING");
            mpNetworkEventListener->receiveDataFromChannel(NetworkScheduler::Channel::OUT_SSID, [this](unsigned int id, bool empty, Bytes data) {
              if (!empty) {
                UNIOT_LOG_DEBUG("SSID: %s", data.terminate().c_str());
              }
            });
            break;

          case NetworkScheduler::DISCONNECTED:
            UNIOT_LOG_DEBUG("AppKit Subscriber, DISCONNECTED");
            break;

          case NetworkScheduler::FAILED:
          default:
            UNIOT_LOG_DEBUG("AppKit Subscriber, FAILED");
            break;
        }
        return;
      }
      if (MQTTKit::CONNECTION == topic) {
        switch (msg) {
          case MQTTKit::SUCCESS:
            UNIOT_LOG_DEBUG("AppKit Subscriber, MQTT SUCCESS");
            mMQTT.renewSubscriptions();
            break;
          case MQTTKit::FAILED:
          default:
            UNIOT_LOG_DEBUG("AppKit Subscriber, MQTT FAILED");
            break;
        }
        return;
      }
    });
  }

  Credentials mCredentials;
  NetworkScheduler mNetwork;
  MQTTKit mMQTT;
  TopDevice mTopDevice;
  LispDevice mLispDevice;

  UniquePointer<NetworkController> mpNetworkDevice;
  UniquePointer<CoreCallbackEventListener> mpNetworkEventListener;
};
}  // namespace uniot
