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

#include <Common.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <IExecutor.h>
#ifdef ESP8266
#include <ESP8266mDNS.h>
#else
#include <ESPmDNS.h>
#endif

#define DNS_PORT 53            ///< Standard DNS port for captive portal functionality
#define HTTP_PORT 80           ///< Standard HTTP port for web server
#define WS_URL "/ws"           ///< WebSocket endpoint URL path
#define DOMAIN_NAME "*"        ///< Wildcard domain name for DNS capture
#define MDNS_HOSTNAME "uniot"  ///< mDNS hostname for local discovery

/**
 * @file ConfigCaptivePortal.h
 * @brief Captive portal implementation for device configuration
 * @defgroup captive_portal Captive Portal
 * @ingroup network
 * @{
 *
 * This file provides a complete captive portal implementation for WiFi device
 * configuration. The captive portal combines DNS redirection, HTTP server,
 * WebSocket communication, and mDNS discovery to create a seamless configuration
 * experience for end users.
 *
 * Key features:
 * - DNS server that redirects all queries to the device's IP
 * - Async HTTP web server for serving configuration interface
 * - WebSocket support for real-time bi-directional communication
 * - mDNS advertisement for local network discovery
 * - Automatic client management and cleanup
 * - Platform-specific optimizations for ESP8266 and ESP32
 *
 * The implementation follows the IExecutor interface pattern, allowing it to
 * be integrated into task-based scheduling systems for non-blocking operation.
 *
 * Example usage:
 * @code
 * IPAddress apIP(192, 168, 4, 1);
 * ConfigCaptivePortal portal(apIP, [](AsyncWebSocket* server,
 *                                     AsyncWebSocketClient* client,
 *                                     AwsEventType type, void* arg,
 *                                     uint8_t* data, size_t len) {
 *   // Handle WebSocket events
 * });
 *
 * if (portal.start()) {
 *   // Portal started successfully
 *   while (running) {
 *     portal.execute(0); // Process requests
 *   }
 * }
 * @endcode
 */

namespace uniot {

/**
 * @brief Extended AsyncWebServer with status monitoring capabilities
 *
 * This class extends the standard AsyncWebServer to provide access to the
 * internal server status, which is useful for monitoring the health and
 * state of the web server during captive portal operations.
 */
class DetailedAsyncWebServer : public AsyncWebServer {
 public:
  /**
   * @brief Construct a new DetailedAsyncWebServer
   * @param port TCP port number for the HTTP server
   */
  DetailedAsyncWebServer(uint16_t port) : AsyncWebServer(port) {}

  /**
   * @brief Get the current server status
   * @retval uint8_t Server status code (0 = stopped, non-zero = running)
   */
  uint8_t status() {
    return _server.status();
  }
};

/**
 * @brief Complete captive portal implementation for device configuration
 *
 * This class implements a full-featured captive portal that combines DNS
 * redirection, HTTP serving, WebSocket communication, and mDNS discovery.
 * It's designed to provide a seamless configuration experience where users
 * connecting to the device's WiFi network are automatically redirected to
 * the configuration interface.
 *
 * The captive portal operates by:
 * 1. Running a DNS server that redirects all queries to the device IP
 * 2. Serving a web-based configuration interface via HTTP
 * 3. Enabling real-time communication through WebSockets
 * 4. Advertising the device via mDNS for local discovery
 *
 * All operations are designed to be non-blocking and suitable for embedded
 * systems with limited resources.
 */
class ConfigCaptivePortal : public IExecutor {
 public:
  /**
   * @brief Construct a new ConfigCaptivePortal
   * @param apIp IP address for the access point and web server
   * @param wsHandler Optional WebSocket event handler callback
   *
   * Initializes all components of the captive portal including DNS server,
   * HTTP server, and WebSocket handler. The WebSocket is automatically
   * added to the HTTP server and configured with the provided event handler.
   */
  ConfigCaptivePortal(const IPAddress& apIp, AwsEventHandler wsHandler = nullptr)
      : mIsStarted(false),
        mMdnsStarted(false),
        mApIp(apIp),
        mpDnsServer(new DNSServer()),
        mpWebServer(new DetailedAsyncWebServer(HTTP_PORT)),
        mpWebSocket(new AsyncWebSocket(WS_URL)),
        mWebSocketHandler(wsHandler),
        mWsClientLastSeen(0) {
    mpWebServer->addHandler(mpWebSocket.get());
  }

  /**
   * @brief Start the captive portal services
   * @retval bool true if all services started successfully, false otherwise
   *
   * Sequentially starts all captive portal components:
   * 1. DNS server for domain redirection
   * 2. WebSocket event handling (if handler provided)
   * 3. HTTP web server
   * 4. mDNS advertisement
   *
   * If any component fails to start, the method returns false and the
   * portal remains in a stopped state.
   */
  bool start() {
    if (!mIsStarted) {
      mpDnsServer->setTTL(30);
      mpDnsServer->setErrorReplyCode(DNSReplyCode::ServerFailure);
      if (mpDnsServer->start(DNS_PORT, DOMAIN_NAME, mApIp)) {
        if (mWebSocketHandler) {
          mpWebSocket->enable(true);
          mpWebSocket->onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
            mWebSocketHandler(server, client, type, arg, data, len);
            mWsClientLastSeen = millis();
          });
        }
        mpWebServer->begin();
        auto status = mpWebServer->status();
        if (status == 0) {
          mpWebServer->end();
          return false;
        }

        if (MDNS.begin(MDNS_HOSTNAME)) {
          MDNS.addService("http", "tcp", HTTP_PORT);
          mMdnsStarted = true;
        }

        return mIsStarted = true;
      }
      return false;
    }
    return true;
  }

  /**
   * @brief Stop all captive portal services
   *
   * Gracefully shuts down all components in reverse order:
   * 1. mDNS service
   * 2. DNS server (with memory cleanup workaround)
   * 3. HTTP web server
   * 4. WebSocket connections
   *
   * Includes workarounds for known memory issues with DNSServer cleanup.
   */
  void stop() {
    if (mIsStarted) {
      if (mMdnsStarted) {
        MDNS.end();
        mMdnsStarted = false;
      }
      // DNSServer has problem with memory deallocation when stop() called
      // TODO: fix, pull request
      mpDnsServer->stop();
      mpDnsServer.reset(new DNSServer());
      mpWebServer->end();
      // mpWebServer->removeHandler(mpWebSocket.get());
      mpWebSocket->closeAll();
      mpWebSocket->cleanupClients();
      mIsStarted = false;
    }
  }

  /**
   * @brief Get the underlying AsyncWebServer instance
   * @retval AsyncWebServer* Pointer to the HTTP server for adding custom routes
   *
   * Provides access to the web server for adding custom request handlers,
   * routes, and other HTTP-specific configuration.
   */
  AsyncWebServer* get() {
    return mpWebServer.get();
  }

  /**
   * @brief Send text message to all connected WebSocket clients
   * @param message Text message to broadcast
   *
   * Broadcasts a text message to all currently connected WebSocket clients.
   * This is commonly used for status updates and configuration responses.
   */
  void wsTextAll(const String& message) {
    if (mpWebSocket) {
      mpWebSocket->textAll(message);
    }
  }

  /**
   * @brief Send text message to a specific WebSocket client
   * @param clientId ID of the target WebSocket client
   * @param message Text message to send
   *
   * Sends a text message to a specific WebSocket client identified by
   * its client ID. Useful for targeted responses and private communication.
   */
  void wsText(uint32_t clientId, const String& message) {
    if (mpWebSocket) {
      mpWebSocket->text(clientId, message);
    }
  }

  /**
   * @brief Close all WebSocket connections
   *
   * Disables the WebSocket server and forcibly closes all active client
   * connections. Used during shutdown or when switching operating modes.
   */
  void wsCloseAll() {
    if (mpWebSocket) {
      mpWebSocket->enable(false);
      mpWebSocket->closeAll();
      mpWebSocket->cleanupClients();
    }
  }

  /**
   * @brief Enable or disable WebSocket functionality
   * @param enable true to enable WebSocket server, false to disable
   *
   * Controls whether the WebSocket server accepts new connections and
   * processes existing ones. Useful for temporarily suspending WebSocket
   * operations without stopping the entire portal.
   */
  void wsEnable(bool enable) {
    if (mpWebSocket) {
      mpWebSocket->enable(enable);
    }
  }

  /**
   * @brief Check if WebSocket clients are actively connected
   * @param window Time window in milliseconds for considering clients active (default: 30 seconds)
   * @retval bool true if clients are connected and recently active, false otherwise
   *
   * Determines if there are WebSocket clients connected and whether they've
   * been active within the specified time window. This helps distinguish
   * between truly active clients and stale connections.
   */
  bool wsClientsActive(unsigned long window = 30000) const {
    auto timeSinceLastSeen = millis() - mWsClientLastSeen;
    return (mpWebSocket && mpWebSocket->count() > 0 && timeSinceLastSeen < window);
  }

  /**
   * @brief Get the IP address of the captive portal
   * @retval const IPAddress& Reference to the portal's IP address
   */
  inline const IPAddress& ip() const {
    return mApIp;
  }

  /**
   * @brief Execute periodic captive portal maintenance tasks
   * @param _ Unused parameter (required by IExecutor interface)
   *
   * Performs essential maintenance operations including:
   * - Processing DNS requests for domain redirection
   * - Cleaning up disconnected WebSocket clients
   * - Updating mDNS service (ESP8266 only)
   *
   * This method should be called regularly from the main loop or task scheduler.
   */
  virtual void execute(short _) override {
    if (mIsStarted) {
      if (mpDnsServer) {
        mpDnsServer->processNextRequest();
      }
      if (mpWebSocket) {
        mpWebSocket->cleanupClients();
      }
#ifdef ESP8266
      if (mMdnsStarted) {
        MDNS.update();
      }
#endif
    }
  }

 private:
  bool mIsStarted;    ///< Flag indicating if the portal is currently running
  bool mMdnsStarted;  ///< Flag indicating if mDNS service is active

  IPAddress mApIp;                                    ///< IP address for the access point and web server
  UniquePointer<DNSServer> mpDnsServer;               ///< DNS server for domain redirection
  UniquePointer<DetailedAsyncWebServer> mpWebServer;  ///< HTTP web server instance
  UniquePointer<AsyncWebSocket> mpWebSocket;          ///< WebSocket server for real-time communication
  AwsEventHandler mWebSocketHandler;                  ///< Callback function for WebSocket events
  unsigned long mWsClientLastSeen;                    ///< Timestamp of last WebSocket client activity
};

}  // namespace uniot

/** @} */
