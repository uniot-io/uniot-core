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

#if defined(ESP8266)
    #include "ESP8266WiFi.h"
    #include <ESP8266mDNS.h>
#elif defined(ESP32)
    #include "WiFi.h"
    #include <ESPmDNS.h>
#endif

#include <Patches.h>
#include <Common.h>
#include <Credentials.h>
#include <TaskScheduler.h>
#include <EventBus.h>
#include <EventEmitter.h>
#include <ConfigCaptivePortal.h>
#include <WifiStorage.h>
#include <config.min.html.gz.h>

namespace uniot {
  const char text_html[]           PROGMEM = "text/html";
  const char text[]                PROGMEM = "text";

//TODO: improve by public methods as needed
  class NetworkScheduler : public ISchedulerConnectionKit, public CoreEventEmitter
  {
  public:
    enum Channel { OUT_SSID = FOURCC(ssid) };
    enum Topic { CONNECTION = FOURCC(netw) };
    enum Msg { FAILED = 0, SUCCESS, CONNECTING, DISCONNECTED, ACCESS_POINT };

    NetworkScheduler(Credentials &credentials)
        : mpCredentials(&credentials),
          mApSubnet(255, 255, 255, 0),
          mConfigServer(IPAddress(1, 1, 1, 1))
    {
      mApName = "UNIOT-" + String(mpCredentials->getShortDeviceId(), HEX);
      mApName.toUpperCase();

      // default wifi persistent storage brings unexpected behavior, I turn it off
      WiFi.persistent(false);
      WiFi.setAutoConnect(false);
      WiFi.setAutoReconnect(false);

#if defined(UNIOT_MODE_AP_STA)
      WiFi.mode(WIFI_AP_STA);
#else
      WiFi.mode(WIFI_STA);
#endif
      _initTasks();
      _initServerCallbacks();
    }

    virtual void pushTo(TaskScheduler &scheduler) override {
      scheduler.push("server_start", mTaskStart);
      scheduler.push("server_serve", mTaskServe);
      scheduler.push("server_stop", mTaskStop);
      scheduler.push("ap_config", mTaskConfigAp);
      scheduler.push("sta_connect", mTaskConnectSta);
      scheduler.push("sta_connecting", mTaskConnecting);
      scheduler.push("wifi_monitor", mTaskMonitoring);
#if defined(UNIOT_MDNS)
      scheduler.push("mdns", mTaskMDNS);
#endif
    }

    virtual void attach() override {
      mWifiStorage.restore();
      if(mWifiStorage.isCredentialsValid()) {
        mTaskConnectSta->attach(500, 1);
      }
#if defined(UNIOT_MODE_AP_STA)
      mTaskConfigAp->attach(500, 1);
#else
      else {
        mTaskConfigAp->attach(500, 1);
      }
#endif
    }

    void forget() {
      UNIOT_LOG_DEBUG("Forget credentials: %s", mWifiStorage.getSsid().c_str());
      mWifiStorage.clean();
      mTaskConfigAp->attach(500, 1);
    }

    bool reconnect() {
      if(mWifiStorage.isCredentialsValid()) {
        mTaskConnectSta->attach(500, 1);
        return true;
      }
      return false;
    }

  private:
    void _initTasks() {
      mTaskStart = TaskScheduler::make([this](SchedulerTask &self, short t) {
        if(mConfigServer.start()) {
          mTaskServe->attach(10);
        }
      });
      mTaskServe = TaskScheduler::make(mConfigServer);
      mTaskStop = TaskScheduler::make([this](SchedulerTask &self, short t) {
        mTaskServe->detach();
        mConfigServer.stop();
      });

      mTaskConfigAp = TaskScheduler::make([this](SchedulerTask &self, short t) {
#if defined(UNIOT_MODE_AP_STA)
        if(WiFi.getMode() != WIFI_AP_STA) {
          WiFi.mode(WIFI_AP_STA);
        }
#else
        WiFi.disconnect(true);
        WiFi.softAPdisconnect(true);
#endif

        if( WiFi.softAPConfig(mConfigServer.ip(), mConfigServer.ip(),  mApSubnet)
          && WiFi.softAP(mApName.c_str()))
        {
#if defined(ESP32) && defined(ENABLE_LOWER_WIFI_TX_POWER)
          WiFi.setTxPower(WIFI_TX_POWER_LEVEL);
#endif
          mTaskStart->attach(500, 1);
          CoreEventEmitter::sendDataToChannel(Channel::OUT_SSID, Bytes(mApName));
          CoreEventEmitter::emitEvent(Topic::CONNECTION, Msg::ACCESS_POINT);
        } else {
          UNIOT_LOG_WARN("NetworkScheduler, mTaskConfigAp failed");
          mTaskConfigAp->attach(500, 1);
        }
      });
      mTaskConnectSta = TaskScheduler::make([this](SchedulerTask &self, short t) {
#if defined(UNIOT_MODE_AP_STA)
        WiFi.disconnect(false, true);
        if(WiFi.getMode() != WIFI_AP_STA) {
          WiFi.mode(WIFI_AP_STA);
        }
#else
        WiFi.disconnect(true, true);
        WiFi.softAPdisconnect(true);
#endif

        WiFi.setHostname(mApName.c_str());
        bool connect = WiFi.begin(mWifiStorage.getSsid().c_str(), mWifiStorage.getPassword().c_str()) != WL_CONNECT_FAILED;
        if (connect)
        {
#if defined(ESP32) && defined(ENABLE_LOWER_WIFI_TX_POWER)
          WiFi.setTxPower(WIFI_TX_POWER_LEVEL);
#endif

#if not defined(UNIOT_MODE_AP_STA)
          mTaskServe->detach();
#endif
          mTaskConnecting->attach(100, 100);
          CoreEventEmitter::sendDataToChannel(Channel::OUT_SSID, Bytes(mWifiStorage.getSsid()));
          CoreEventEmitter::emitEvent(Topic::CONNECTION, Msg::CONNECTING);
        }
      });
      mTaskConnecting = TaskScheduler::make([this](SchedulerTask &self, short times) {
        auto __processFailure = [this]() {
          const int triesBeforeGivingUp = 3;
          static int tries = 0;
          if(++tries < triesBeforeGivingUp) {
            UNIOT_LOG_INFO("Tries to connect until give up is %d", triesBeforeGivingUp - tries);
            mTaskConnectSta->attach(500, 1);
          } else {
            tries = 0;
#if not defined(UNIOT_MODE_AP_STA)
            mTaskConfigAp->attach(500, 1);
#endif
            CoreEventEmitter::emitEvent(Topic::CONNECTION, Msg::FAILED);
          }
        };

        switch(WiFi.status()){
          case WL_CONNECTED:
          mTaskConnecting->detach();
#if not defined(UNIOT_MODE_AP_STA)
          mTaskStop->attach(500, 1);
#endif
          mTaskMonitoring->attach(2000);
          mWifiStorage.store();
          mpCredentials->store(); // NOTE: it is stored each time it is connected to the network

#if defined(UNIOT_MDNS)
          // Start mDNS responder to enable UNIOT-XXXX.local domain access
          if (MDNS.begin(mApName.c_str())) {
            mTaskMDNS->attach(100);
            MDNS.addService("http", "tcp", 80);
            UNIOT_LOG_INFO("mDNS started: %s.local", mApName.c_str());
          } else {
            UNIOT_LOG_WARN("Failed to start mDNS");
          }
#endif
          CoreEventEmitter::emitEvent(Topic::CONNECTION, Msg::SUCCESS);
          break;

          case WL_NO_SSID_AVAIL:
          case WL_CONNECT_FAILED:
          mTaskConnecting->detach();
          __processFailure();
          break;

          case WL_DISCONNECTED:
          if(!times) {
            __processFailure();
          }
          break;

          case WL_IDLE_STATUS:
          default:
          break;
        }
      });
      mTaskMonitoring = TaskScheduler::make([this](SchedulerTask &self, short times) {
        if(WiFi.status() != WL_CONNECTED) {
          mTaskMonitoring->detach();

#if defined(UNIOT_MDNS)
          // Stop mDNS when disconnected from WiFi
          mTaskMDNS->detach();
          MDNS.end();
#endif

          CoreEventEmitter::emitEvent(Topic::CONNECTION, Msg::DISCONNECTED);
        }
      });

#if defined(UNIOT_MDNS)
      mTaskMDNS = TaskScheduler::make([this] (SchedulerTask &self, short times) {
        MDNS.update();
      });
#endif
    }

    void _initServerCallbacks() {
      mConfigServer.get()->onNotFound([this] {
        mConfigServer.get()->sendHeader("Location", "/", true);
        mConfigServer.get()->send_P(307, text, text);
      });

      mConfigServer.get()->on("/", [this] {
        mConfigServer.get()->sendHeader("Content-Encoding", "gzip");
        mConfigServer.get()->send_P(200, text_html, CONFIG_MIN_HTML_GZ, CONFIG_MIN_HTML_GZ_LENGTH);
      });

      mConfigServer.get()->on("/scan", [this] {
        // TODO: implement async scan
        auto n = WiFi.scanNetworks();
        String networks = "[";
        for (auto i = 0; i < n; ++i) {
          networks += '"' + WiFi.SSID(i) + '"';
          if (i < n - 1)
            networks += ',';
        }
        networks += ']';
        UNIOT_LOG_DEBUG("Networks: %s", networks.c_str());
        mConfigServer.get()->send(200, text, networks);
      });

      mConfigServer.get()->on("/config", [this] {
        mWifiStorage.setCredentials(mConfigServer.get()->arg("ssid"), mConfigServer.get()->arg("pass"));
        mConfigServer.get()->sendHeader("Location", "/", true);
        mConfigServer.get()->send_P(307, text, text);
        if(mWifiStorage.isCredentialsValid()) {
          mTaskConnectSta->attach(500, 1);
          mpCredentials->setOwnerId(mConfigServer.get()->arg("acc"));
        }
      });
    }

    Credentials *mpCredentials;
    WifiStorage mWifiStorage;

    String mApName;
    IPAddress mApSubnet;
    ConfigCaptivePortal mConfigServer;

    TaskScheduler::TaskPtr mTaskStart;
    TaskScheduler::TaskPtr mTaskServe;
    TaskScheduler::TaskPtr mTaskStop;
    TaskScheduler::TaskPtr mTaskConfigAp;
    TaskScheduler::TaskPtr mTaskConnectSta;
    TaskScheduler::TaskPtr mTaskConnecting;
    TaskScheduler::TaskPtr mTaskMonitoring;

#if defined(UNIOT_MDNS)
    TaskScheduler::TaskPtr mTaskMDNS;
#endif
  };
}
