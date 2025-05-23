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

#include <WiFiNetworkScan.h>
#include <Patches.h>
#include <Common.h>
#include <Credentials.h>
#include <TaskScheduler.h>
#include <EventBus.h>
#include <EventEmitter.h>
#include <ConfigCaptivePortal.h>
#include <WifiStorage.h>
#include <config.min.html.gz.h>
#include <MicroJSON.h>

namespace uniot {
  class NetworkScheduler : public ISchedulerConnectionKit, public CoreEventEmitter
  {
  public:
    enum Channel { OUT_SSID = FOURCC(ssid) };
    enum Topic { CONNECTION = FOURCC(netw) };
    enum Msg { FAILED = 0, SUCCESS, CONNECTING, DISCONNECTING, DISCONNECTED, ACCESS_POINT };

    NetworkScheduler(Credentials &credentials)
        : mpCredentials(&credentials),
          mApSubnet(255, 255, 255, 0),
          mConfigServer(IPAddress(1, 1, 1, 1),
                        [this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
                          _handleWebSocketEvent(server, client, type, arg, data, len);
                        })
    {
      mApName = "UNIOT-" + String(mpCredentials->getShortDeviceId(), HEX);
      mApName.toUpperCase();
      mCanScan = true;
      mLastSaveResult = -1;

      // default wifi persistent storage brings unexpected behavior, I turn it off
      WiFi.persistent(false);
      WiFi.setAutoConnect(false);
      WiFi.setAutoReconnect(false);
      WiFi.setHostname(mApName.c_str());

      _initTasks();
    }

    virtual void pushTo(TaskScheduler &scheduler) override {
      scheduler.push("server_start", mTaskStart);
      scheduler.push("server_serve", mTaskServe);
      scheduler.push("server_stop", mTaskStop);
      scheduler.push("ap_config", mTaskConfigAp);
      scheduler.push("ap_stop", mTaskStopAp);
      scheduler.push("sta_connect", mTaskConnectSta);
      scheduler.push("sta_connecting", mTaskConnecting);
      scheduler.push("wifi_monitor", mTaskMonitoring);
      scheduler.push("wifi_scan", mTaskScan);
#if defined(ESP32)
      scheduler.push("wifi_scan_complete", mWifiScan.getTask());
#endif
    }

    virtual void attach() override {
      mWifiStorage.restore();
      if(mWifiStorage.isCredentialsValid()) {
        mTaskConnectSta->once(500);
      } else {
        mTaskConfigAp->once(500);
      }
    }

    void forget() {
      UNIOT_LOG_DEBUG("Forget credentials: %s", mWifiStorage.getSsid().c_str());
      mWifiStorage.clean();
      CoreEventEmitter::emitEvent(Topic::CONNECTION, Msg::DISCONNECTING);
      mTaskConfigAp->once(500);
    }

    bool reconnect() {
      if(mWifiStorage.isCredentialsValid()) {
        CoreEventEmitter::emitEvent(Topic::CONNECTION, Msg::DISCONNECTING);
        mTaskConnectSta->once(500);
        return true;
      }
      return false;
    }

  private:
    enum ACTIONS {
      INVALID = 0,
      STATUS = 100,
      SAVE,
      SCAN,
      ASK
    };

    void _initTasks() {
      mTaskStart = TaskScheduler::make([this](SchedulerTask &self, short t) {
        mTaskStop->detach();
        if(mConfigServer.start()) {
          _initServerCallbacks();
          mTaskServe->attach(10);
        } else {
          UNIOT_LOG_WARN("Start server failed. Restarting...");
          self.once(1000);
        }
      });
      mTaskServe = TaskScheduler::make(mConfigServer);
      mTaskStop = TaskScheduler::make([this](SchedulerTask &self, short t) {
        static bool wsClosed = false;
        UNIOT_LOG_DEBUG("Stop server, state: %d", wsClosed);
        // 1: close websocket
        // 2: stop access point
        // 3: stop server
        if(!wsClosed) {
          mConfigServer.wsCloseAll();
          wsClosed = true;
          self.once(10000);
          return;
        }
        mTaskServe->detach();
        mConfigServer.stop();
        mConfigServer.reset();
        wsClosed = false;
        mLastNetworks = static_cast<const char *>(nullptr); // invalidate String
      });

      mTaskConfigAp = TaskScheduler::make([this](SchedulerTask &self, short t) {
        WiFi.disconnect(true, true);
        mTaskStopAp->detach();
        if( WiFi.softAPConfig(mConfigServer.ip(), mConfigServer.ip(),  mApSubnet)
          && WiFi.softAP(mApName.c_str()))
        {
#if defined(ESP32) && defined(ENABLE_LOWER_WIFI_TX_POWER)
          WiFi.setTxPower(WIFI_TX_POWER_LEVEL);
#endif
          mTaskStart->once(500);
          mTaskScan->once(500);
          CoreEventEmitter::sendDataToChannel(Channel::OUT_SSID, Bytes(mApName));
          CoreEventEmitter::emitEvent(Topic::CONNECTION, Msg::ACCESS_POINT);
        } else {
          UNIOT_LOG_WARN("Start server failed");
          mTaskConfigAp->attach(500, 1);
        }
      });
      mTaskStopAp = TaskScheduler::make([this](SchedulerTask &self, short t) {
        WiFi.softAPdisconnect(true); // check with 8266
      });

      mTaskConnectSta = TaskScheduler::make([this](SchedulerTask &self, short t) {
        WiFi.disconnect(false, true);
        bool connect = WiFi.begin(mWifiStorage.getSsid().c_str(), mWifiStorage.getPassword().c_str()) != WL_CONNECT_FAILED;
        if (connect)
        {
#if defined(ESP32) && defined(ENABLE_LOWER_WIFI_TX_POWER)
          WiFi.setTxPower(WIFI_TX_POWER_LEVEL);
#endif
          mTaskConnecting->attach(100, 50);
          CoreEventEmitter::sendDataToChannel(Channel::OUT_SSID, Bytes(mWifiStorage.getSsid()));
          CoreEventEmitter::emitEvent(Topic::CONNECTION, Msg::CONNECTING);
          mCanScan = false;
          mLastSaveResult = -1;
        }
      });
      mTaskConnecting = TaskScheduler::make([this](SchedulerTask &self, short times) {
        auto __processFailure = [this](int triesBeforeGivingUp = 3) {
          static int tries = 0;
          if(++tries < triesBeforeGivingUp) {
            UNIOT_LOG_INFO("Tries to connect until give up is %d", triesBeforeGivingUp - tries);
            mTaskConnectSta->attach(500, 1);
          } else {
            tries = 0;
            CoreEventEmitter::emitEvent(Topic::CONNECTION, Msg::FAILED);
            mCanScan = true;
            mLastSaveResult = 0;
          }
        };

        switch(WiFi.status()){
          case WL_CONNECTED:
          self.detach();
          mTaskMonitoring->attach(200);
          mWifiStorage.store();
          if (mpCredentials->isOwnerChanged()) {
            mpCredentials->store();
          }

          mCanScan = true;
          mLastSaveResult = 1;
          mTaskStop->once(30000);
          mTaskStopAp->once(35000);
          CoreEventEmitter::emitEvent(Topic::CONNECTION, Msg::SUCCESS);
          break;

          case WL_NO_SSID_AVAIL:
          case WL_CONNECT_FAILED:
          self.detach();
          __processFailure();
          break;
#if defined(ESP8266)
          case WL_WRONG_PASSWORD:
          self.detach();
          __processFailure(1);
          break;
#endif
          case WL_IDLE_STATUS:
          case WL_DISCONNECTED:
          if(!times) {
            __processFailure();
          }
          break;

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

      mTaskScan = TaskScheduler::make([this] (SchedulerTask &self, short times) {
        static auto __broadcastNets = [this](const String &netJsonArray) {
          String nets;
          JSON::Object(nets)
              .put("nets", netJsonArray, false)
              .close();
          mConfigServer.wsTextAll(nets);
          delay(50);  // to allow for all clients to receive the message (relevant for ESP32)
        };
        if (mCanScan) {
          mWifiScan.scanNetworksAsync([this](int n) {
            mLastNetworks = static_cast<const char *>(nullptr); // invalidate String
            JSON::Array jsonNets(mLastNetworks);
            for (auto i = 0; i < n; ++i) {
              jsonNets.appendArray()
                  .append(WiFi.BSSIDstr(i))
                  .append(WiFi.SSID(i))
                  .append(WiFi.RSSI(i))
                  .append(mWifiScan.isSecured(WiFi.encryptionType(i)))
                  .close();
            }
            jsonNets.close();
            WiFi.scanDelete();
            __broadcastNets(mLastNetworks);
          });
        } else {
          __broadcastNets(mLastNetworks);
        }
      });
    }

    void _initServerCallbacks() {
      auto server = mConfigServer.get();
      if (server) {
        server->onNotFound([](AsyncWebServerRequest *request) {
          auto response = request->beginResponse(307);
          response->addHeader("Location", "/");
          request->send(response);
        });

        server->on("/", [this](AsyncWebServerRequest *request) {
          auto response = request->beginResponse(200, "text/html", CONFIG_MIN_HTML_GZ, CONFIG_MIN_HTML_GZ_LENGTH, nullptr);
          response->addHeader("Content-Encoding", "gzip");
          request->send(response);
        });
      }
    }

    void _handleWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
      switch (type) {
        case WS_EVT_CONNECT:
          // mAppState.Network.WebSocketsClients = mWebSocket.count();
          UNIOT_LOG_INFO("WebSocket client #%u connected from %s", client->id(), client->remoteIP().toString().c_str());
          break;
        case WS_EVT_DISCONNECT:
          // mAppState.Network.WebSocketsClients = mWebSocket.count();
          UNIOT_LOG_INFO("WebSocket client #%u disconnected", client->id());
          break;
        case WS_EVT_DATA:
          _handleWebSocketMessage(client->id(), arg, data, len);
          break;
        default:
          break;
      }
    }

    void _handleWebSocketMessage(uint32_t clientId, void *arg, uint8_t *data, size_t len) {
      auto *info = (AwsFrameInfo *)arg;
      if (info->opcode == WS_BINARY) {
        CBORObject msg(Bytes(data, len));
        if (!msg.hasError()) {
          auto action = msg.getInt("action");
          if (action != ACTIONS::INVALID) {
            switch (action) {
              case ACTIONS::STATUS: {
                String status;
                JSON::Object(status)
                    .put("id", mpCredentials->getDeviceId())
                    .put("acc", mpCredentials->getOwnerId())
                    .put("nets", mLastNetworks.length() ? mLastNetworks : "[]", false)
                    .put("homeNet", WiFi.isConnected() ? WiFi.SSID() : "")
                    .close();
                mConfigServer.wsTextAll(status);
                break;
              }
              case ACTIONS::SAVE: {
                mWifiStorage.setCredentials(msg.getString("ssid"), msg.getString("pass"));
                if(mWifiStorage.isCredentialsValid()) {
                  mTaskConnectSta->once(500);
                  mpCredentials->setOwnerId(msg.getString("acc"));
                  UNIOT_LOG_DEBUG("Is owner changed: %d", mpCredentials->isOwnerChanged());
                }
                break;
              }
              case ACTIONS::SCAN: {
                mTaskScan->once(1000);
                break;
              }
              case ACTIONS::ASK: {
                if (mLastSaveResult > -1) {
                  String success;
                  JSON::Object(success)
                    .put("success", mLastSaveResult)
                    .close();
                  mConfigServer.wsText(clientId, success);
                }
                break;
              }
              default:
                break;
            }
          } else {
            UNIOT_LOG_WARN("WebSocket message is not a valid action");
          }
        } else {
          UNIOT_LOG_WARN("WebSocket message is not a valid CBOR");
        }
      }
    }

    Credentials *mpCredentials;
    WifiStorage mWifiStorage;

    String mApName;
    IPAddress mApSubnet;
    ConfigCaptivePortal mConfigServer;

    String mLastNetworks;
    int8_t mLastSaveResult;
    bool mCanScan;

    TaskScheduler::TaskPtr mTaskStart;
    TaskScheduler::TaskPtr mTaskServe;
    TaskScheduler::TaskPtr mTaskStop;
    TaskScheduler::TaskPtr mTaskConfigAp;
    TaskScheduler::TaskPtr mTaskStopAp;
    TaskScheduler::TaskPtr mTaskConnectSta;
    TaskScheduler::TaskPtr mTaskConnecting;
    TaskScheduler::TaskPtr mTaskMonitoring;
    TaskScheduler::TaskPtr mTaskScan;

#if defined(ESP32)
    ESP32WifiScan mWifiScan;
#elif defined(ESP8266)
    ESP8266WifiScan mWifiScan;
#endif
  };
} // namespace uniot
