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
#elif defined(ESP32)
    #include "WiFi.h"
#endif

#include <Common.h>
#include <Credentials.h>
#include <TaskScheduler.h>
#include <EventEmitter.h>
#include <ConfigCaptivePortal.h>
#include <WifiStorage.h>
#include <ILightPrint.h>
#include <config.min.html.gz.h>

namespace uniot {
//---------------------------------------------"________________"
  const char strApCreated[]        PROGMEM = "Access Point:";
  const char strStaConnecting[]    PROGMEM = "Connecting to:";
  const char strFailed[]           PROGMEM = "> Failed";
  const char strSuccess[]          PROGMEM = "> Success";

  const char text_html[]           PROGMEM = "text/html";
  const char text[]                PROGMEM = "text";

//TODO: improve by public methods as needed
  class NetworkScheduler : public ISchedulerConnectionKit, public CoreEventEmitter
  {
  public:
    enum Topic { CONNECTION = FOURCC(netw) };
    enum Msg { FAILED = 0, SUCCESS, CONNECTING, DISCONNECTED, ACCESS_POINT };

    NetworkScheduler(Credentials &credentials)
        : NetworkScheduler(credentials, nullptr) {}

    NetworkScheduler(Credentials &credentials, ILightPrint *printer)
        : mpCredentials(&credentials),
          mpPrinter(printer),
          mpApIp(std::make_shared<IPAddress>(1, 1, 1, 1)),
          mpApSubnet(new IPAddress(255, 255, 255, 0)),
          mpConfigServer(mpApIp)
    {
      mApName = "UNIOT-" + String(mpCredentials->getShortDeviceId(), HEX);
      mApName.toUpperCase();

      // default wifi persistent storage brings unexpected behavior, I turn it off
      WiFi.persistent(false);
      WiFi.setAutoConnect(false);
      WiFi.setAutoReconnect(false);

      WiFi.mode(WIFI_STA);
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
    }

    virtual void attach() override {
      mWifiStorage.restore();
      if(mWifiStorage.getWifiArgs()->isValid()) {
        _printp(strStaConnecting);
        _print(mWifiStorage.getWifiArgs()->ssid.c_str());
        mTaskConnectSta->attach(500, 1);
      } else {
        mTaskConfigAp->attach(500, 1);
      }
    }

    void forget() {
      mWifiStorage.clean();
      mTaskConfigAp->attach(500, 1);
    }

    bool reconnect() {
      if(mWifiStorage.getWifiArgs()->isValid()) {
        _printp(strStaConnecting);
        _print(mWifiStorage.getWifiArgs()->ssid.c_str());
        mTaskConfigAp->detach();
        mTaskConnectSta->attach(500, 1);
        return true;
      }
      return false;
    }

  private:
    inline void _printp(const char* PSTR_str) {
      if(mpPrinter) {
        mpPrinter->printlnPGM(PSTR_str);
      }
    }
    inline void _print(const char* str){
      if(mpPrinter) {
        mpPrinter->println(str);
      }
    }

    void _initTasks() {
      mTaskStart = TaskScheduler::make([this](SchedulerTask &self, short t) {
        if(mpConfigServer.start()) {
          mTaskServe->attach(10);
        }
      });
      mTaskServe = TaskScheduler::make(mpConfigServer);
      mTaskStop = TaskScheduler::make([this](SchedulerTask &self, short t) {
        mTaskServe->detach();
        mpConfigServer.stop();
      });

      mTaskConfigAp = TaskScheduler::make([this](SchedulerTask &self, short t) {
        WiFi.disconnect(true);
        if( WiFi.softAPConfig((*mpApIp), (*mpApIp),  (*mpApSubnet))
          && WiFi.softAP(mApName.c_str()))
        {
          _printp(strApCreated);
          _print(mApName.c_str());
          mTaskStart->attach(500, 1);
          emitEvent(Topic::CONNECTION, Msg::ACCESS_POINT);
        } else {
          UNIOT_LOG_WARN("NetworkScheduler, mTaskConfigAp failed");
          mTaskConfigAp->attach(500, 1);
        }
      });
      mTaskConnectSta = TaskScheduler::make([this](SchedulerTask &self, short t) {
        WiFi.disconnect(true);
        bool disconect = true;
        WiFi.softAPdisconnect(true); // !!!!!!!!!!!!
        bool connect = WiFi.begin(mWifiStorage.getWifiArgs()->ssid.c_str(), mWifiStorage.getWifiArgs()->pass.c_str()) != WL_CONNECT_FAILED;
        if (disconect && connect)
        {
          mTaskServe->detach();
          mTaskConnecting->attach(100, 100);
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
            mTaskConfigAp->attach(500, 1);
            CoreEventEmitter::emitEvent(Topic::CONNECTION, Msg::FAILED);
            _printp(strFailed);
          }
        };

        switch(WiFi.status()){
          case WL_CONNECTED:
          mTaskConnecting->detach();
          mTaskStop->attach(500, 1);
          mTaskMonitoring->attach(2000);
          mWifiStorage.store();
          mpCredentials->store(); // NOTE: it is stored each time it is connected to the network
          _printp(strSuccess);
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
          CoreEventEmitter::emitEvent(Topic::CONNECTION, Msg::DISCONNECTED);
        }
      });
    }

    void _initServerCallbacks() {
      mpConfigServer.get()->onNotFound([this] {
        mpConfigServer.get()->sendHeader("Location", "/", true);
        mpConfigServer.get()->send_P(307, text, text);
      });

      mpConfigServer.get()->on("/", [this] {
        mpConfigServer.get()->sendHeader("Content-Encoding", "gzip");
        mpConfigServer.get()->send_P(200, text_html, CONFIG_MIN_HTML_GZ, CONFIG_MIN_HTML_GZ_LENGTH);
      });

      mpConfigServer.get()->on("/scan", [this] {
        auto n = WiFi.scanNetworks();
        String networks = "[";
        for (auto i = 0; i < n; ++i) {
          networks += '"' + WiFi.SSID(i) + '"';
          if (i < n - 1)
            networks += ',';
        }
        networks += ']';
        UNIOT_LOG_DEBUG("Networks: %s", networks.c_str());
        mpConfigServer.get()->send(200, text, networks);
      });

      mpConfigServer.get()->on("/config", [this] {
        mWifiStorage.getWifiArgs()->ssid = mpConfigServer.get()->arg("ssid");
        mWifiStorage.getWifiArgs()->pass = mpConfigServer.get()->arg("pass");
        mpConfigServer.get()->sendHeader("Location", "/", true);
        mpConfigServer.get()->send_P(307, text, text);
        if(mWifiStorage.getWifiArgs()->isValid()) {
          _printp(strStaConnecting);
          _print(mWifiStorage.getWifiArgs()->ssid.c_str());
          mTaskConnectSta->attach(500, 1);

          mpCredentials->setOwnerId(mpConfigServer.get()->arg("acc"));
        }
      });
    }

    Credentials *mpCredentials;
    WifiStorage mWifiStorage;
    String mApName;

    ILightPrint* mpPrinter;

    SharedPointer<IPAddress> mpApIp;
    UniquePointer<IPAddress> mpApSubnet;
    ConfigCaptivePortal mpConfigServer;

    TaskScheduler::TaskPtr mTaskStart;
    TaskScheduler::TaskPtr mTaskServe;
    TaskScheduler::TaskPtr mTaskStop;
    TaskScheduler::TaskPtr mTaskConfigAp;
    TaskScheduler::TaskPtr mTaskConnectSta;
    TaskScheduler::TaskPtr mTaskConnecting;
    TaskScheduler::TaskPtr mTaskMonitoring;
  };
}
