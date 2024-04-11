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

#include <CBORStorage.h>
#include <EventListener.h>
#include <MQTTDevice.h>
#include <unLisp.h>

namespace uniot {

class LispDevice : public MQTTDevice, public CBORStorage, public CoreEventListener {
 public:
  LispDevice()
      : MQTTDevice(),
        CBORStorage("lisp.cbor"),
        CoreEventListener(),
        mChecksum(0),
        mPersist(false),
        mFailedWithError(false) {
    CoreEventListener::listenToEvent(unLisp::Topic::OUT_LISP_MSG);
    CoreEventListener::listenToEvent(unLisp::Topic::OUT_LISP_REQUEST);
  }

  virtual void syncSubscriptions() override {
    mTopicScript = MQTTDevice::subscribeDevice("script");
    mTopicEvents = MQTTDevice::subscribeGroup("all", "event/+");
  }

  unLisp &getLisp() {
    return unLisp::getInstance();
  }

  void runStoredCode() {
    if (CBORStorage::restore()) {
      auto code = object().getString("code");
      mPersist = object().getInt("persist");
      mChecksum = object().getInt("checksum");

      if (mPersist && code.length() > 0) {
        getLisp().runCode(code);
      }
    }
  }

  bool store() {
    object()
        .put("persist", mPersist)
        .put("checksum", (int)mChecksum)
        .put("code", mPersist ? getLisp().getLastCode().c_str() : "")
        .forceDirty();  // Avoid optimization. The pointer to the data may be the same. Read the warnings.

    return CBORStorage::store();
  }

  virtual void onEventReceived(unsigned int topic, int msg) override {
    if (topic == unLisp::Topic::OUT_LISP_MSG) {
      if (msg == unLisp::Msg::OUT_MSG_ERROR) {
        CoreEventListener::receiveDataFromChannel(unLisp::Channel::OUT_LISP_ERR, [this](unsigned int id, bool empty, Bytes data) {
          if (!empty) {
            mFailedWithError = true;
            publishDevice("script/error", data);
            UNIOT_LOG_ERROR("lisp error: %s", data.c_str());
          }
        });
        return;
      }
      if (msg == unLisp::Msg::OUT_MSG_ADDED) {
        CoreEventListener::receiveDataFromChannel(unLisp::Channel::OUT_LISP, [](unsigned int id, bool empty, Bytes data) {
          // NOTE: it is currently only used for debugging purposes
          // UNIOT_LOG_INFO_IF(!empty, "lisp: %s", data.toString().c_str());
        });
        return;
      }
      return;
    }
    if (topic == unLisp::Topic::OUT_LISP_REQUEST) {
      if (msg == unLisp::Msg::OUT_REFRESH_EVENTS) {
        // This is necessary to retrieve events marked as `retained` during the execution of a new script
        this->unsubscribe(mTopicEvents);
        this->subscribe(mTopicEvents);
        return;
      }
      return;
    }
  }

  virtual void handle(const String &topic, const Bytes &payload) override {
    if (MQTTDevice::isTopicMatch(mTopicScript, topic)) {
      handleScript(payload);
      return;
    }
    if (MQTTDevice::isTopicMatch(mTopicEvents, topic)) {
      handleEvent(payload);
      return;
    }
  }

  void handleScript(const Bytes &payload) {
    static bool firstPacketReceived = false;
    auto packet = CBORObject(payload);
    auto script = Bytes(packet.getString("code"));
    auto newPersist = packet.getBool("persist");
    auto newChecksum = script.terminate().checksum();

    auto ignoreScript = false;

    if (!firstPacketReceived) {
      auto isEqual = mChecksum == newChecksum;
      ignoreScript = isEqual && mPersist && !mFailedWithError;
      firstPacketReceived = true;
    }

    if (!ignoreScript) {
      mChecksum = newChecksum;
      mPersist = newPersist;
      mFailedWithError = false;

      // TODO: check signature here later
      getLisp().runCode(script);
      LispDevice::store();
    } else {
      UNIOT_LOG_INFO("script ignored: %s", script.c_str());
    }

    // let's free some memory
    object().clean();
    // NOTE: you may need getLasCode() somewhere else, but it has already cleaned here
    getLisp().cleanLastCode();
  }

  void handleEvent(const Bytes &payload) {
    if (payload.size() > 0) {
      // TODO: implement event validation
      CoreEventListener::sendDataToChannel(unLisp::Channel::IN_EVENT, payload);
      CoreEventListener::emitEvent(unLisp::Topic::IN_LISP_EVENT, unLisp::Msg::IN_NEW_EVENT);
    }
  }

 private:
  uint32_t mChecksum;
  bool mPersist;
  bool mFailedWithError;

  String mTopicScript;
  String mTopicEvents;
};

}  // namespace uniot
