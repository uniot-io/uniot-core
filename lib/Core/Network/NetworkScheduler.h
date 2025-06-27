/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2025 Uniot <contact@uniot.io>
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
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#elif defined(ESP32)
#include <ESPmDNS.h>
#include <WiFi.h>
#endif

#include <Common.h>
#include <ConfigCaptivePortal.h>
#include <Credentials.h>
#include <EventBus.h>
#include <EventEmitter.h>
#include <MicroJSON.h>
#include <NetworkEvents.h>
#include <Patches.h>
#include <TaskScheduler.h>
#include <WiFiNetworkScan.h>
#include <WifiStorage.h>
#include <config.min.html.gz.h>

/**
 * @file NetworkScheduler.h
 * @brief Complete WiFi network management and configuration system
 * @defgroup network_scheduler Network Scheduler
 * @ingroup network
 * @{
 *
 * This file provides a comprehensive WiFi network management system that handles:
 * - WiFi station connection and reconnection
 * - Access Point configuration mode
 * - Web-based configuration interface with WebSocket communication
 * - Network scanning and availability monitoring
 * - Credential storage and validation
 * - Automatic fallback between STA and AP modes
 *
 * The NetworkScheduler manages the complete lifecycle of WiFi connectivity,
 * from initial configuration through ongoing connection monitoring. It provides
 * a captive portal for device configuration when no valid credentials are stored
 * or when the configured network becomes unavailable.
 *
 * Features:
 * - Task-based asynchronous operation
 * - WebSocket-based real-time configuration interface
 * - Automatic network availability checking
 * - Graceful handling of connection failures
 * - Platform-specific optimizations for ESP8266 and ESP32
 *
 * Example usage:
 * @code
 * Credentials credentials;
 * NetworkScheduler networkScheduler(credentials);
 * TaskScheduler scheduler;
 *
 * networkScheduler.pushTo(scheduler);
 * networkScheduler.attach();
 *
 * // In main loop
 * scheduler.run();
 * @endcode
 */

namespace uniot {
/**
 * @brief Complete WiFi network management and configuration scheduler
 *
 * This class orchestrates all aspects of WiFi connectivity including station
 * connection, access point configuration mode, web-based setup interface,
 * and ongoing network monitoring. It implements the ISchedulerConnectionKit
 * interface and emits network events through the CoreEventEmitter system.
 *
 * The scheduler operates in multiple modes:
 * - STA mode: Connects to configured WiFi networks
 * - AP mode: Creates configuration access point
 * - Hybrid mode: Maintains AP while attempting STA connection
 *
 * All operations are task-based and non-blocking, making it suitable for
 * real-time embedded applications.
 */
class NetworkScheduler : public ISchedulerConnectionKit, public CoreEventEmitter {
 public:
  /**
   * @brief Construct a new NetworkScheduler
   * @param credentials Reference to device credentials manager
   *
   * Initializes the network scheduler with default configuration including:
   * - AP name generation based on device ID
   * - WiFi persistence and auto-connect disabled
   * - Task initialization for all network operations
   * - WebSocket configuration server setup
   */
  NetworkScheduler(Credentials &credentials)
      : mpCredentials(&credentials),
        mApSubnet(255, 255, 255, 0),
        mConfigServer(IPAddress(1, 1, 1, 1),
          [this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
            _handleWebSocketEvent(server, client, type, arg, data, len);
          }) {
    mApName = "UNIOT-" + String(mpCredentials->getShortDeviceId(), HEX);
    mApName.toUpperCase();
    mCanScan = true;
    mApEnabled = false;
    mLastSaveResult = -1;

    // default wifi persistent storage brings unexpected behavior, I turn it off
    WiFi.persistent(false);
    WiFi.setAutoConnect(false);
    WiFi.setAutoReconnect(false);
    WiFi.setHostname(mApName.c_str());

    _initTasks();
  }

  /**
   * @brief Push all network tasks to the scheduler
   * @param scheduler TaskScheduler instance to receive the tasks
   *
   * Registers all network management tasks including:
   * - Server start/serve/stop tasks
   * - AP configuration and stop tasks
   * - STA connection and monitoring tasks
   * - WiFi scanning and availability check tasks
   */
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
    scheduler.push("wifi_check", mTaskAvailabilityCheck);
#if defined(ESP32)
    scheduler.push("wifi_scan_complete", mWifiScan.getTask());
#endif
  }

  /**
   * @brief Attach the network scheduler and start initial connection
   *
   * Restores stored WiFi credentials and initiates either STA connection
   * (if valid credentials exist) or AP configuration mode (if no valid
   * credentials are stored).
   */
  virtual void attach() override {
    mWifiStorage.restore();
    if (mWifiStorage.isCredentialsValid()) {
      mTaskConnectSta->once(500);
    } else {
      mTaskConfigAp->once(500);
    }
  }

  /**
   * @brief Start or recover configuration mode
   *
   * Initiates AP configuration mode or recovers an existing AP if already
   * running. This method is called when manual configuration is requested
   * or when automatic connection fails.
   */
  void config() {
    if (_tryToRecoverAp()) {
      UNIOT_LOG_DEBUG("Config already in progress. AP recovered");
      return;
    }
    mTaskConfigAp->once(100);
  }

  /**
   * @brief Forget stored WiFi credentials and enter configuration mode
   *
   * Clears stored network credentials and switches to AP configuration mode.
   * Emits disconnecting event to notify other system components.
   */
  void forget() {
    UNIOT_LOG_DEBUG("Forget credentials: %s", mWifiStorage.getSsid().c_str());
    mWifiStorage.clean();
    CoreEventEmitter::emitEvent(events::network::Topic::CONNECTION, events::network::Msg::DISCONNECTING);
    mTaskConfigAp->once(500);
  }

  /**
   * @brief Attempt to reconnect using stored credentials
   * @retval bool true if reconnection attempt was started, false if no valid credentials
   *
   * Initiates reconnection to the stored WiFi network if valid credentials
   * exist. Recovers AP mode if it was previously active.
   */
  bool reconnect() {
    if (mWifiStorage.isCredentialsValid()) {
      CoreEventEmitter::emitEvent(events::network::Topic::CONNECTION, events::network::Msg::DISCONNECTING);
      mTaskConnectSta->once(500);

      if (_tryToRecoverAp()) {
        UNIOT_LOG_DEBUG("Reconnecting while AP is enabled. AP recovered");
      }

      return true;
    }
    return false;
  }

  /**
   * @brief Set and store new WiFi credentials
   * @param ssid Network SSID
   * @param password Network password
   * @retval bool true if credentials were set successfully, false if SSID is empty
   *
   * Validates and stores new WiFi credentials for future connection attempts.
   * The credentials are immediately persisted to storage.
   */
  bool setCredentials(const String &ssid, const String &password) {
    if (!ssid.isEmpty()) {
      mWifiStorage.setCredentials(ssid, password);
      mWifiStorage.store();
      return true;
    }
    return false;
  }

 private:
  /**
   * @brief WebSocket message action types
   *
   * Defines the supported actions that can be received via WebSocket
   * messages from the configuration interface.
   */
  enum ACTIONS {
    INVALID = 0,   ///< Invalid or unrecognized action
    STATUS = 100,  ///< Request current device and network status
    SAVE,          ///< Save new WiFi credentials
    SCAN,          ///< Request WiFi network scan
    ASK            ///< Query last save operation result
  };

  /**
   * @brief Initialize all network management tasks
   *
   * Creates and configures all scheduled tasks for network operations including
   * server management, AP/STA modes, connection monitoring, and network scanning.
   * Each task is configured with appropriate callbacks and scheduling parameters.
   */
  void _initTasks() {
    // Server lifecycle tasks
    mTaskStart = TaskScheduler::make([this](SchedulerTask &self, short t) {
      mTaskStop->detach();
      if (mConfigServer.start()) {
        _initServerCallbacks();
        mConfigServer.wsEnable(true);  // Ensure that WS are enabled after disabling them in "Step 1 of Stopping Configuration" during the AP recovery process
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
      if (!wsClosed) {
        mConfigServer.wsCloseAll();
        wsClosed = true;
        self.once(10000);  // Stopping Configuration. Step 3. Carefully change the deferrals.
        return;
      }
      mTaskServe->detach();
      mConfigServer.stop();
      wsClosed = false;
      mLastNetworks = static_cast<const char *>(nullptr);  // invalidate String
    });

    // Access Point tasks
    mTaskConfigAp = TaskScheduler::make([this](SchedulerTask &self, short t) {
      WiFi.disconnect(true, true);
      mTaskStopAp->detach();
      if (WiFi.softAPConfig(mConfigServer.ip(), mConfigServer.ip(), mApSubnet) && WiFi.softAP(mApName.c_str())) {
#if defined(ESP32) && defined(ENABLE_LOWER_WIFI_TX_POWER)
        WiFi.setTxPower(WIFI_TX_POWER_LEVEL);
#endif
        mTaskStart->once(500);
        mTaskScan->once(500);
        mTaskAvailabilityCheck->attach(10000);
        mApEnabled = true;
        CoreEventEmitter::sendDataToChannel(events::network::Channel::OUT_SSID, Bytes(mApName));
        CoreEventEmitter::emitEvent(events::network::Topic::CONNECTION, events::network::Msg::ACCESS_POINT);
      } else {
        UNIOT_LOG_WARN("Start server failed");
        mTaskConfigAp->attach(500, 1);
      }
    });

    mTaskStopAp = TaskScheduler::make([this](SchedulerTask &self, short t) {
      mApEnabled = false;
      WiFi.softAPdisconnect(true);  // check with 8266
    });

    // Station connection tasks
    mTaskConnectSta = TaskScheduler::make([this](SchedulerTask &self, short t) {
      WiFi.disconnect(false, true);
      bool connect = WiFi.begin(mWifiStorage.getSsid().c_str(), mWifiStorage.getPassword().c_str()) != WL_CONNECT_FAILED;
      if (connect) {
#if defined(ESP32) && defined(ENABLE_LOWER_WIFI_TX_POWER)
        WiFi.setTxPower(WIFI_TX_POWER_LEVEL);
#endif
        mTaskConnecting->attach(100, 50);
        CoreEventEmitter::sendDataToChannel(events::network::Channel::OUT_SSID, Bytes(mWifiStorage.getSsid()));
        CoreEventEmitter::emitEvent(events::network::Topic::CONNECTION, events::network::Msg::CONNECTING);
        mCanScan = false;
        mLastSaveResult = -1;
      } else {
        mTaskConnecting->detach();
        CoreEventEmitter::emitEvent(events::network::Topic::CONNECTION, events::network::Msg::FAILED);
        mCanScan = true;
        mLastSaveResult = 0;
      }
    });

    mTaskConnecting = TaskScheduler::make([this](SchedulerTask &self, short times) {
      auto __processFailure = [this](int triesBeforeGivingUp = 3) {
        static int tries = 0;
        if (++tries < triesBeforeGivingUp) {
          UNIOT_LOG_INFO("Tries to connect until give up is %d", triesBeforeGivingUp - tries);
          mTaskConnectSta->attach(500, 1);
        } else {
          tries = 0;
          mWifiStorage.restore();
          CoreEventEmitter::emitEvent(events::network::Topic::CONNECTION, events::network::Msg::FAILED);
          mCanScan = true;
          mLastSaveResult = 0;
        }
      };

      switch (WiFi.status()) {
        case WL_CONNECTED:
          self.detach();
          mTaskMonitoring->attach(200);
          mWifiStorage.store();
          if (mpCredentials->isOwnerChanged()) {
            mpCredentials->store();
          }

          mCanScan = true;
          mLastSaveResult = 1;
          mTaskStop->once(30000);    // Stopping Configuration. Step 1. Carefully change the deferrals.
          mTaskStopAp->once(35000);  // Stopping Configuration. Step 2. Carefully change the deferrals.
          mTaskAvailabilityCheck->detach();
          CoreEventEmitter::emitEvent(events::network::Topic::CONNECTION, events::network::Msg::SUCCESS);
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
        case WL_CONNECTION_LOST:
          if (!times) {
            __processFailure();
          }
          break;

        default:
          UNIOT_LOG_WARN("Unexpected WiFi status: %d", WiFi.status());
          break;
      }
    });

    mTaskMonitoring = TaskScheduler::make([this](SchedulerTask &self, short times) {
      if (WiFi.status() != WL_CONNECTED) {
        mTaskMonitoring->detach();
        CoreEventEmitter::emitEvent(events::network::Topic::CONNECTION, events::network::Msg::DISCONNECTED);
      }
    });

    // Network scanning tasks
    mTaskScan = TaskScheduler::make([this](SchedulerTask &self, short times) {
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
          mLastNetworks = static_cast<const char *>(nullptr);  // invalidate String
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

    mTaskAvailabilityCheck = TaskScheduler::make([this](SchedulerTask &self, short times) {
      static int scanInProgressFuse = 0;
      if (scanInProgressFuse-- > 0) {
        UNIOT_LOG_INFO("Availability check skipped, scan in progress");
        return;
      }

      if (mCanScan &&
          !mConfigServer.wsClientsActive() &&
          mWifiStorage.isCredentialsValid()) {
        UNIOT_LOG_INFO("Checking availability of the network [%s]", mWifiStorage.getSsid().c_str());
        scanInProgressFuse = 3;

        mWifiScan.scanNetworksAsync([&](int n) {
          scanInProgressFuse = 0;
          if (self.isAttached() &&
              mCanScan &&
              !mConfigServer.wsClientsActive() &&
              mWifiStorage.isCredentialsValid()) {
            for (auto i = 0; i < n; ++i) {
              if (WiFi.SSID(i) == mWifiStorage.getSsid()) {
                UNIOT_LOG_INFO("Network [%s] is available", WiFi.SSID(i).c_str());
                CoreEventEmitter::emitEvent(events::network::Topic::CONNECTION, events::network::Msg::AVAILABLE);
                break;
              }
            }
          } else {
            UNIOT_LOG_INFO("Scan done, skipping availability check");
          }
          WiFi.scanDelete();
        });
      }
    });
  }

  /**
   * @brief Initialize HTTP server route callbacks
   *
   * Sets up the web server routes for the configuration interface including
   * the main configuration page and redirect handling for captive portal
   * functionality.
   */
  void _initServerCallbacks() {
    auto server = mConfigServer.get();
    if (server) {
      server->onNotFound([](AsyncWebServerRequest *request) {
        // auto response = request->beginResponse(307);
        // response->addHeader("Location", "/");
        // request->send(response);
        request->redirect("http://uniot.local/");
      });

      server->on("/", [this](AsyncWebServerRequest *request) {
        auto response = request->beginResponse(200, "text/html", CONFIG_MIN_HTML_GZ, CONFIG_MIN_HTML_GZ_LENGTH, nullptr);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
      });
    }
  }

  /**
   * @brief Handle WebSocket connection events
   * @param server WebSocket server instance
   * @param client WebSocket client instance
   * @param type Event type (connect, disconnect, data, etc.)
   * @param arg Event argument data
   * @param data Message data payload
   * @param len Length of message data
   *
   * Processes WebSocket lifecycle events including client connections,
   * disconnections, and incoming data messages.
   */
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

  /**
   * @brief Process incoming WebSocket messages
   * @param clientId ID of the WebSocket client
   * @param arg Message frame information
   * @param data Message payload data
   * @param len Length of message payload
   *
   * Parses CBOR-encoded WebSocket messages and executes the requested actions
   * including status requests, credential saving, network scanning, and
   * result queries.
   */
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
              if (mWifiStorage.isCredentialsValid()) {
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

  /**
   * @brief Attempt to recover an existing access point
   * @retval bool true if AP was recovered, false if no AP was active
   *
   * Restores the configuration server and tasks if an access point is
   * already running. Used when switching back to configuration mode
   * while maintaining the existing AP.
   */
  bool _tryToRecoverAp() {
    if (mApEnabled) {
      // mTaskStop->detach();
      mTaskStart->once(100);
      mTaskStopAp->detach();
      mConfigServer.wsEnable(true);
      mTaskAvailabilityCheck->attach(10000);
      return true;
    }
    return false;
  }

  Credentials *mpCredentials;  ///< Pointer to device credentials manager
  WifiStorage mWifiStorage;    ///< WiFi credentials storage handler

  String mApName;                     ///< Generated access point name
  IPAddress mApSubnet;                ///< Subnet mask for AP mode
  ConfigCaptivePortal mConfigServer;  ///< Configuration web server with captive portal

  String mLastNetworks;    ///< Cached JSON string of last network scan results
  int8_t mLastSaveResult;  ///< Result of last credential save operation (-1: none, 0: failed, 1: success)
  bool mCanScan;           ///< Flag indicating if network scanning is allowed
  bool mApEnabled;         ///< Flag indicating if access point is currently active

  // Task pointers for all network operations
  TaskScheduler::TaskPtr mTaskStart;              ///< Task for starting configuration server
  TaskScheduler::TaskPtr mTaskServe;              ///< Task for serving web requests
  TaskScheduler::TaskPtr mTaskStop;               ///< Task for stopping configuration server
  TaskScheduler::TaskPtr mTaskConfigAp;           ///< Task for configuring access point
  TaskScheduler::TaskPtr mTaskStopAp;             ///< Task for stopping access point
  TaskScheduler::TaskPtr mTaskConnectSta;         ///< Task for initiating station connection
  TaskScheduler::TaskPtr mTaskConnecting;         ///< Task for monitoring connection progress
  TaskScheduler::TaskPtr mTaskMonitoring;         ///< Task for monitoring established connections
  TaskScheduler::TaskPtr mTaskScan;               ///< Task for WiFi network scanning
  TaskScheduler::TaskPtr mTaskAvailabilityCheck;  ///< Task for checking configured network availability

#if defined(ESP32)
  ESP32WifiScan mWifiScan;  ///< ESP32-specific WiFi scanner
#elif defined(ESP8266)
  ESP8266WifiScan mWifiScan;  ///< ESP8266-specific WiFi scanner
#endif
};
}  // namespace uniot

/** @} */
