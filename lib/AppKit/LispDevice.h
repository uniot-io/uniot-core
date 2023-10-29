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
  LispDevice() : MQTTDevice(), CBORStorage("lisp.cbor"), CoreEventListener(), mFirstPacketReceived(false) {
    CoreEventListener::listenToEvent(unLisp::Topic::LISP_OUT);
  }

  void syncSubscriptions() override {
    mTopicScript = MQTTDevice::subscribeDevice("script");
    mTopicEvents = MQTTDevice::subscribeGroup("all", "event/+");
  }

  unLisp &getLisp() {
    return unLisp::getInstance();
  }

  void runStoredCode() {
    if (CBORStorage::restore()) {
      auto code = object().getString("code");
      auto persist = object().getInt("persist");
      mChecksum = object().getInt("checksum");

      if (persist && code.length() > 0)
        getLisp().runCode(code);
    }
  }

  bool store() {
    mChecksum = getLisp().getLastCode().checksum();
    object()
        .put("persist", getLisp().isLastCodePersist())
        .put("checksum", (int)mChecksum);

    if (getLisp().isLastCodePersist())
      object()
          .put("code", getLisp().getLastCode().c_str())
          .forceDirty();  // Avoid optimization. The pointer to the data may be the same. Read the warnings.
    return CBORStorage::store();
  }

  virtual void onEventReceived(unsigned int topic, int msg) override {
    if (msg == unLisp::ERROR) {
      auto lastError = getLisp().getLastError();
      publishDevice("script_error", lastError);
      UNIOT_LOG_ERROR("lisp error: %s", lastError.c_str());
    } else if (msg == unLisp::MSG_ADDED) {
      CoreEventListener::receiveDataFromChannel(unLisp::Channel::LISP_OUTPUT, [](unsigned int id, bool empty, Bytes data) {
        UNIOT_LOG_INFO("event bus id: %d, channel empty: %d, lisp: %s", id, empty, data.toString().c_str());
      });
    }
  }

  void handle(const String &topic, const Bytes &payload) override {
    UNIOT_LOG_INFO("lisp device handle: %s", topic.c_str());
    if (MQTTDevice::isTopicMatch(mTopicScript, topic)) {
      UNIOT_LOG_INFO("script received");
      handleScript(payload);
      return;
    }
    if (MQTTDevice::isTopicMatch(mTopicEvents, topic)) {
      UNIOT_LOG_INFO("event received");
      handleEvent(payload);
      return;
    }
  }

  void handleScript(const Bytes &payload) {
    auto packet = CBORObject(payload);
    auto script = Bytes(packet.getString("code"));

    auto newChecksum = script.terminate().checksum();
    auto isEqual = mChecksum == newChecksum;
    auto ignoreScript = isEqual;

    if (mFirstPacketReceived) {
      ignoreScript = isEqual && getLisp().isLastCodePersist();
    } else {
      mFirstPacketReceived = true;
    }

    if (!ignoreScript) {
      // TODO: check signature here later
      getLisp().runCode(script);

      if (!isEqual) {
        LispDevice::store();
      }
    } else {
      UNIOT_LOG_INFO("script ignored: %s", script.c_str());
    }

    // let's free some memory
    object().clean();
    // NOTE: you may need getLasCode() somewhere else, but it has already cleaned here
    getLisp().cleanLastCode();
  }

  void handleEvent(const Bytes &payload) {
    CoreEventListener::sendDataToChannel(unLisp::Channel::EVENT, payload);
    CoreEventListener::emitEvent(unLisp::Topic::LISP_EVENT, unLisp::Msg::NEW_EVENT);
  }

 private:
  bool mFirstPacketReceived;
  uint32_t mChecksum;

  String mTopicScript;
  String mTopicEvents;
};

}  // namespace uniot
