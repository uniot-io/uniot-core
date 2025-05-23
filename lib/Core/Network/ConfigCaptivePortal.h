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

#include <Common.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <IExecutor.h>

#define DNS_PORT 53
#define HTTP_PORT 80
#define WS_URL "/ws"
#define DOMAIN_NAME "*"

namespace uniot {
class DetailedAsyncWebServer : public AsyncWebServer {
 public:
  DetailedAsyncWebServer(uint16_t port) : AsyncWebServer(port) {}

  uint8_t status() {
    return _server.status();
  }
};

class ConfigCaptivePortal : public IExecutor {
 public:
  ConfigCaptivePortal(const IPAddress& apIp, AwsEventHandler wsHandler = nullptr)
      : mIsStarted(false),
        mApIp(apIp),
        mpDnsServer(new DNSServer()),
        mpWebServer(new DetailedAsyncWebServer(HTTP_PORT)),
        mpWebSocket(new AsyncWebSocket(WS_URL)),
        mWebSocketHandler(wsHandler) {
    mpWebServer->addHandler(mpWebSocket.get());
  }

  bool start() {
    if (!mIsStarted) {
      mpDnsServer->setTTL(300);
      mpDnsServer->setErrorReplyCode(DNSReplyCode::ServerFailure);
      if (mpDnsServer->start(DNS_PORT, DOMAIN_NAME, mApIp)) {
        if (mWebSocketHandler) {
          mpWebSocket->enable(true);
          mpWebSocket->onEvent(mWebSocketHandler);
        }
        mpWebServer->begin();
        auto status = mpWebServer->status();
        if (status == 0) {
          mpWebServer->end();
          return false;
        }
        return mIsStarted = true;
      }
      return false;
    }
    return true;
  }

  void stop() {
    if (mIsStarted) {
      // DNSServer has problem with memory deallocation when stop() called
      // TODO: fix, pull request
      mpDnsServer->stop();
      mpWebServer->end();
      // mpWebServer->removeHandler(mpWebSocket.get());
      mpWebSocket->closeAll();
      mpWebSocket->cleanupClients();
      mIsStarted = false;
    }
  }

  void reset() {
    mpDnsServer.reset(new DNSServer());
    // mpWebServer.reset(new AsyncWebServer(HTTP_PORT));
    // mpWebSocket.reset(new AsyncWebSocket(WS_URL));
    mIsStarted = false;
  }

  AsyncWebServer* get() {
    return mpWebServer.get();
  }

  void wsTextAll(const String& message) {
    if (mpWebSocket) {
      mpWebSocket->textAll(message);
    }
  }

  void wsText(uint32_t clientId, const String& message) {
    if (mpWebSocket) {
      mpWebSocket->text(clientId, message);
    }
  }

  void wsCloseAll() {
    if (mpWebSocket) {
      mpWebSocket->enable(false);
      mpWebSocket->closeAll();
      mpWebSocket->cleanupClients();
    }
  }

  inline const IPAddress& ip() const {
    return mApIp;
  }

  virtual void execute(short _) override {
    if (mIsStarted) {
      if (mpDnsServer) {
        mpDnsServer->processNextRequest();
      }
      if (mpWebSocket) {
        mpWebSocket->cleanupClients();
      }
    }
  }

 private:
  bool mIsStarted;

  IPAddress mApIp;
  UniquePointer<DNSServer> mpDnsServer;
  UniquePointer<DetailedAsyncWebServer> mpWebServer;
  UniquePointer<AsyncWebSocket> mpWebSocket;
  AwsEventHandler mWebSocketHandler;
};
}  // namespace uniot
