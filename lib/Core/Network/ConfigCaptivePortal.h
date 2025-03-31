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
#define DOMAIN_NAME "*"

namespace uniot {
class ConfigCaptivePortal : public IExecutor {
 public:
  ConfigCaptivePortal(const IPAddress& apIp)
      : mIsStarted(false),
        mApIp(apIp),
        mpDnsServer(new DNSServer()),
        mpWebServer(new AsyncWebServer(HTTP_PORT)) {}

  bool start() {
    if (!mIsStarted) {
      mpDnsServer->setTTL(300);
      mpDnsServer->setErrorReplyCode(DNSReplyCode::ServerFailure);
      if (mpDnsServer->start(DNS_PORT, DOMAIN_NAME, mApIp)) {
        mpWebServer->begin();
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
      mIsStarted = false;
    }
  }

  void reset() {
    mpDnsServer.reset(new DNSServer());
    mpWebServer.reset(new AsyncWebServer(HTTP_PORT));
    mIsStarted = false;
  }

  AsyncWebServer* get() {
    return mpWebServer.get();
  }

  inline const IPAddress& ip() const {
    return mApIp;
  }

  virtual void execute(short _) override {
    if (mIsStarted) {
      mpDnsServer->processNextRequest();
    }
  }

 private:
  bool mIsStarted;

  IPAddress mApIp;
  UniquePointer<DNSServer> mpDnsServer;
  UniquePointer<AsyncWebServer> mpWebServer;
};
}  // namespace uniot
