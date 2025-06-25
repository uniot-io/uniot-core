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

/** @cond */
/**
 * DO NOT DELETE THE "app-kit" GROUP DEFINITION BELOW.
 * Used to create the Application Kit topic in the documentation. If you want to delete this file,
 * please paste the group definition into another file and delete this one.
 */
/** @endcond */

/**
 * @defgroup app-kit Application Kit
 * @brief Application Kit for Uniot Core
 *
 * This module provides the main application toolkit that integrates various components of the Uniot system.
 * It manages network connections, MQTT communication, and device management.
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

/** @cond */
/**
 * DO NOT DELETE THE FOLLOWING DESCRIPTION OF THE NAMESPACE.
 * This is used to describe the namespace in the documentation. If you want to delete this file,
 * please paste the namespace description into another file and delete this one.
 */
/** @endcond */

/**
 * @namespace uniot
 * @brief Contains all classes and functions related to the Uniot Core.
 */
namespace uniot {
/**
 * @brief Main application toolkit that integrates various components of the Uniot system.
 * @defgroup app-kit-main AppKit
 * @ingroup app-kit
 * @{
 *
 * AppKit is a singleton class that provides a unified interface for managing core system components:
 * - Network connections via NetworkScheduler
 * - MQTT communication via MQTTKit
 * - Lisp interpreter integration via unLisp
 * - Device management via TopDevice and LispDevice
 *
 * It implements ICoreEventBusConnectionKit for event bus integration and ISchedulerConnectionKit
 * for task scheduling integration.
 */
class AppKit : public ICoreEventBusConnectionKit, public ISchedulerConnectionKit, public Singleton<AppKit> {
  friend class Singleton<AppKit>;

 public:
  /**
   * @struct NetworkControllerConfig
   * @brief Configuration parameters for NetworkController
   *
   * This structure holds all parameters needed to configure the NetworkController component,
   * including pin assignments and reboot behavior settings.
   */
  struct NetworkControllerConfig {
    uint8_t pinBtn = UINT8_MAX;       ///< Button pin (UINT8_MAX means not used)
    uint8_t activeLevelBtn = LOW;     ///< Active level for button (LOW or HIGH)
    uint8_t pinLed = UINT8_MAX;       ///< LED pin (UINT8_MAX means not used)
    uint8_t activeLevelLed = HIGH;    ///< Active level for LED (LOW or HIGH)
    uint8_t maxRebootCount = 3;       ///< Maximum number of consecutive reboots
    uint32_t rebootWindowMs = 10000;  ///< Time window in ms for counting reboots
    bool registerLispBtn = true;      ///< Whether to register the button with the Lisp interpreter
  };

  /**
   * @brief Get the Lisp interpreter instance
   * @retval unLisp& Reference to the unLisp instance
   */
  unLisp &getLisp() {
    return unLisp::getInstance();
  }

  /**
   * @brief Get the MQTT communication kit instance
   * @retval MQTTKit& Reference to the MQTTKit instance
   */
  MQTTKit &getMQTT() {
    return mMQTT;
  }

  /**
   * @brief Get the device credentials
   * @retval Credentials& Reference to the Credentials instance
   */
  const Credentials &getCredentials() {
    return mCredentials;
  }

  bool setWiFiCredentials(const String &ssid, const String &password) {
    return mNetwork.setCredentials(ssid, password);
  }

  /**
   * @brief Add all managed tasks to the scheduler
   *
   * Implements ISchedulerConnectionKit interface to register all component tasks
   * with the provided scheduler.
   *
   * @param scheduler The TaskScheduler to add tasks to
   */
  virtual void pushTo(TaskScheduler &scheduler) override {
    scheduler.push(mNetwork);
    scheduler.push(mMQTT);
    scheduler.push("lisp_task", getLisp().getTask());
    scheduler.push("lisp_cleanup", getLisp().getCleanupTask());

    mTopDevice.setScheduler(scheduler);

    if (mpNetworkDevice) {
      scheduler.push(*mpNetworkDevice);
    } else {
      UNIOT_LOG_WARN("Configure Network Controller before pushing to the scheduler");
    }
  }

  /**
   * @brief Initialize and attach all components
   *
   * Implements ISchedulerConnectionKit interface to initialize all components
   * including primitives, network, MQTT, and device controllers.
   */
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

    getLisp().getCleanupTask()->attach(15000);
    mLispDevice.runStoredCode();
  }

  /**
   * @brief Register all components with the event bus
   *
   * Implements ICoreEventBusConnectionKit interface to register all components
   * with the provided event bus. Opens necessary data channels and registers entities.
   *
   * @param eventBus The CoreEventBus to register with
   */
  virtual void registerWithBus(CoreEventBus &eventBus) override {
    eventBus.openDataChannel(events::network::Channel::OUT_SSID, 1);
    eventBus.openDataChannel(events::lisp::Channel::OUT_LISP, 5);
    eventBus.openDataChannel(events::lisp::Channel::OUT_LISP_LOG, 10);
    eventBus.openDataChannel(events::lisp::Channel::OUT_LISP_ERR, 1);
    eventBus.openDataChannel(events::lisp::Channel::OUT_EVENT, 10);
    eventBus.openDataChannel(events::lisp::Channel::IN_EVENT, 20);
    eventBus.registerEntity(&Date::getInstance());
    eventBus.registerEntity(&mNetwork);
    eventBus.registerEntity(&mMQTT);
    eventBus.registerEntity(&getLisp());
    eventBus.registerEntity(&mLispDevice);
    eventBus.registerEntity(mpNetworkEventListener
                                ->listenToEvent(events::network::Topic::CONNECTION)
                                ->listenToEvent(events::mqtt::Topic::CONNECTION));

    if (mpNetworkDevice) {
      eventBus.registerEntity(mpNetworkDevice.get());
    } else {
      UNIOT_LOG_WARN("Configure Network Controller before registering to the event bus");
    }
  }

  /**
   * @brief Unregister all components from the event bus
   *
   * Implements ICoreEventBusConnectionKit interface to unregister all components
   * from the provided event bus. Closes data channels and unregisters entities.
   *
   * @param eventBus The CoreEventBus to unregister from
   */
  virtual void unregisterFromBus(CoreEventBus &eventBus) override {
    eventBus.closeDataChannel(events::network::Channel::OUT_SSID);
    eventBus.closeDataChannel(events::lisp::Channel::OUT_LISP);
    eventBus.closeDataChannel(events::lisp::Channel::OUT_LISP_LOG);
    eventBus.closeDataChannel(events::lisp::Channel::OUT_LISP_ERR);
    eventBus.closeDataChannel(events::lisp::Channel::OUT_EVENT);
    eventBus.closeDataChannel(events::lisp::Channel::IN_EVENT);
    eventBus.unregisterEntity(&Date::getInstance());
    eventBus.unregisterEntity(&mNetwork);
    eventBus.unregisterEntity(&mMQTT);
    eventBus.unregisterEntity(&getLisp());
    eventBus.unregisterEntity(&mLispDevice);
    eventBus.unregisterEntity(mpNetworkEventListener
                                  ->stopListeningToEvent(events::network::Topic::CONNECTION)
                                  ->stopListeningToEvent(events::mqtt::Topic::CONNECTION));

    if (mpNetworkDevice) {
      eventBus.unregisterEntity(mpNetworkDevice.get());
    }
  }

  /**
   * @brief Configure the NetworkController using a config structure
   *
   * Convenience method to configure the NetworkController using a single config structure.
   * Creates and initializes the NetworkController instance if not already configured.
   *
   * @param config The NetworkControllerConfig structure containing all parameters
   */
  void configureNetworkController(const NetworkControllerConfig &config) {
    configureNetworkController(config.pinBtn,
                               config.activeLevelBtn,
                               config.pinLed,
                               config.activeLevelLed,
                               config.maxRebootCount,
                               config.rebootWindowMs,
                               config.registerLispBtn);
  }

  /**
   * @brief Configure the NetworkController with individual parameters
   *
   * Creates and initializes the NetworkController instance with the specified parameters
   * if not already configured. Also links button click events to Lisp primitives.
   *
   * @param pinBtn Button pin (UINT8_MAX means not used)
   * @param activeLevelBtn Active level for button (LOW or HIGH)
   * @param pinLed LED pin (UINT8_MAX means not used)
   * @param activeLevelLed Active level for LED (LOW or HIGH)
   * @param maxRebootCount Maximum number of consecutive reboots
   * @param rebootWindowMs Time window in ms for counting reboots
   */
  void configureNetworkController(uint8_t pinBtn = UINT8_MAX,
                                  uint8_t activeLevelBtn = LOW,
                                  uint8_t pinLed = UINT8_MAX,
                                  uint8_t activeLevelLed = HIGH,
                                  uint8_t maxRebootCount = 3,
                                  uint32_t rebootWindowMs = 10000,
                                  bool registerLispBtn = true) {
    if (mpNetworkDevice) {
      UNIOT_LOG_WARN("Network Controller already configured");
      return;
    }

    mpNetworkDevice = MakeUnique<NetworkController>(mNetwork, pinBtn, activeLevelBtn, pinLed, activeLevelLed, maxRebootCount, rebootWindowMs);
    auto ctrlBtn = mpNetworkDevice->getButton();
    if (ctrlBtn && registerLispBtn) {
      PrimitiveExpeditor::getRegisterManager().link(primitive::name::bclicked, ctrlBtn, FOURCC(ctrl));
    }
  }

  void setLispEventInterceptor(LispEventInterceptor interceptor) {
    mLispDevice.setEventInterceptor(interceptor);
  }

  void publishLispEvent(const String &eventID, int32_t value) {
    mLispDevice.publishLispEvent(eventID, value);
  }

 private:
  /**
   * @brief Private constructor for singleton pattern
   *
   * Initializes all components including NetworkScheduler, MQTTKit, and event listeners.
   * Calls initialization methods for MQTT, tasks and event listeners.
   */
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
          info.put("lisp_heap", UNIOT_LISP_HEAP);
        }),
        mpNetworkDevice(nullptr) {
    _initMqtt();
    _initTasks();
    _initListeners();
  }

  /**
   * @brief Initialize MQTT server connection and device subscriptions
   *
   * Sets up the MQTT server connection and adds devices to the MQTT kit.
   * Synchronizes device subscriptions.
   */
  inline void _initMqtt() {
    // TODO: should I move configs to the Credentials class?
    mMQTT.setServer("mqtt.uniot.io", 1883);
    mMQTT.addDevice(mTopDevice);
    mMQTT.addDevice(mLispDevice);
    mTopDevice.syncSubscriptions();
    mLispDevice.syncSubscriptions();
  }

  /**
   * @brief Initialize system tasks
   *
   * Placeholder for initializing additional tasks as needed.
   */
  inline void _initTasks() {
    // TODO: init new tasks here
  }

  /**
   * @brief Initialize Lisp primitives
   *
   * Registers available primitives with the Lisp interpreter based on
   * primitive registers.
   */
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

  /**
   * @brief Initialize event listeners
   *
   * Sets up event listeners for network and MQTT events. Handles connection
   * state changes and logs relevant information.
   */
  inline void _initListeners() {
    mpNetworkEventListener = MakeUnique<CoreCallbackEventListener>([&](int topic, int msg) {
      if (events::network::Topic::CONNECTION == topic) {
        switch (msg) {
          case events::network::Msg::SUCCESS:
            UNIOT_LOG_DEBUG("AppKit Subscriber, SUCCESS, ip: %s", WiFi.localIP().toString().c_str());
            break;
          case events::network::Msg::ACCESS_POINT:
            UNIOT_LOG_DEBUG("AppKit Subscriber, ACCESS_POINT");
            mpNetworkEventListener->receiveDataFromChannel(events::network::Channel::OUT_SSID, [this](unsigned int id, bool empty, Bytes data) {
              if (!empty) {
                UNIOT_LOG_DEBUG("SSID: %s", data.terminate().c_str());
              }
            });
            break;

          case events::network::Msg::CONNECTING:
            UNIOT_LOG_DEBUG("AppKit Subscriber, CONNECTING");
            mpNetworkEventListener->receiveDataFromChannel(events::network::Channel::OUT_SSID, [this](unsigned int id, bool empty, Bytes data) {
              if (!empty) {
                UNIOT_LOG_DEBUG("SSID: %s", data.terminate().c_str());
              }
            });
            break;

          case events::network::Msg::DISCONNECTING:
            UNIOT_LOG_DEBUG("AppKit Subscriber, DISCONNECTING");
            break;

          case events::network::Msg::DISCONNECTED:
            UNIOT_LOG_DEBUG("AppKit Subscriber, DISCONNECTED");
            break;

          case events::network::Msg::AVAILABLE:
            UNIOT_LOG_DEBUG("AppKit Subscriber, AVAILABLE");
            break;

          case events::network::Msg::FAILED:
          default:
            UNIOT_LOG_DEBUG("AppKit Subscriber, FAILED");
            break;
        }
        return;
      }
      if (events::mqtt::Topic::CONNECTION == topic) {
        switch (msg) {
          case events::mqtt::Msg::SUCCESS:
            UNIOT_LOG_DEBUG("AppKit Subscriber, MQTT SUCCESS");
            if (mCredentials.isOwnerChanged()) {
              UNIOT_LOG_INFO("Owner changed, renewing subscriptions");
              mMQTT.renewSubscriptions();
              mCredentials.resetOwnerChanged();
            } else {
              UNIOT_LOG_INFO("Owner not changed, do not renew subscriptions");
            }
            break;
          case events::mqtt::Msg::FAILED:
          default:
            UNIOT_LOG_DEBUG("AppKit Subscriber, MQTT FAILED");
            break;
        }
        return;
      }
    });
  }

  Credentials mCredentials;   ///< Device credentials
  NetworkScheduler mNetwork;  ///< Network connection manager
  MQTTKit mMQTT;              ///< MQTT communication manager
  TopDevice mTopDevice;       ///< Top-level device manager
  LispDevice mLispDevice;     ///< Lisp interpreter device interface

  UniquePointer<NetworkController> mpNetworkDevice;                 ///< Network control interface (optional)
  UniquePointer<CoreCallbackEventListener> mpNetworkEventListener;  ///< Network event listener
};
/** @} */
}  // namespace uniot
