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
#include <Date.h>
#include <EventListener.h>
#include <MQTTDevice.h>
#include <unLisp.h>

#include <functional>

namespace uniot {

class LispDevice : public MQTTDevice, public CBORStorage, public CoreEventListener {
 public:
  struct LispEvent {
    struct Sender {
      String type;
      String id;
    } sender;
    String eventID;
    int32_t value;
    uint64_t timestamp;
  };
  using LispEventInterceptor = std::function<bool(const LispEvent &event)>;

  LispDevice()
      : MQTTDevice(),
        CBORStorage("lisp.cbor"),
        CoreEventListener(),
        mEventInterceptor(nullptr),
        mChecksum(0),
        mPersist(false),
        mFailedWithError(false) {
    CoreEventListener::listenToEvent(unLisp::Topic::OUT_LISP_MSG);
    CoreEventListener::listenToEvent(unLisp::Topic::OUT_LISP_REQUEST);
    CoreEventListener::listenToEvent(unLisp::Topic::OUT_LISP_EVENT);
  }

  virtual void syncSubscriptions() override {
    mTopicScript = MQTTDevice::subscribeDevice("script");
    mTopicEvents = MQTTDevice::subscribeGroup("all", "event/+");
  }

  unLisp &getLisp() {
    return unLisp::getInstance();
  }

  void setEventInterceptor(LispEventInterceptor interceptor) {
    mEventInterceptor = interceptor;
  }

  void publishLispEvent(const String &eventID, int32_t value) {
    CBORObject event;
    event.put("eventID", eventID.c_str());
    event.put("value", value);
    _populateAndPublishEvent(event.build());
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

            CBORObject packet;
            packet.put("type", "error");
            packet.put("timestamp", static_cast<int64_t>(Date::now()));
            packet.put("msg", data.c_str());
            publishDevice("debug/err", packet.build(), true);
            UNIOT_LOG_ERROR("lisp error: %s", data.c_str());
          }
        });
        return;
      }
      if (msg == unLisp::Msg::OUT_MSG_LOG) {
        CoreEventListener::receiveDataFromChannel(unLisp::Channel::OUT_LISP_LOG, [this](unsigned int id, bool empty, Bytes data) {
          if (!empty) {
            CBORObject packet;
            packet.put("type", "log");
            packet.put("timestamp", static_cast<int64_t>(Date::now()));
            packet.put("msg", data.c_str());
            publishDevice("debug/log", packet.build());
            UNIOT_LOG_INFO("lisp log: %s", data.c_str());
          }
        });
        return;
      }
      if (msg == unLisp::Msg::OUT_MSG_ADDED) {
        CoreEventListener::receiveDataFromChannel(unLisp::Channel::OUT_LISP, [](unsigned int id, bool empty, Bytes data) {
          // NOTE: it is currently only used for debugging purposes
          // UNIOT_LOG_TRACE_IF(!empty, "lisp: %s", data.toString().c_str());
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
    if (topic == unLisp::Topic::OUT_LISP_EVENT) {
      if (msg == unLisp::Msg::OUT_NEW_EVENT) {
        CoreEventListener::receiveDataFromChannel(unLisp::Channel::OUT_EVENT, [this](unsigned int id, bool empty, Bytes data) {
          if (!empty) {
            _populateAndPublishEvent(data);
          }
        });
      }
      return;
    }
  }

  virtual void handle(const String &topic, const Bytes &payload) override {
    if (MQTTDevice::isTopicMatch(mTopicScript, topic)) {
      MQTTDevice::publishEmptyDevice("debug/err");  // clear previous errors
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
    CBORObject packet(payload);
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
    getLisp().cleanLastCode();  // Is it safe?
  }

  void handleEvent(const Bytes &payload) {
    if (payload.size() > 0) {
      CBORObject event(payload);
      auto eventID = event.getString("eventID");
      auto valueStr = event.getValueAsString("value");

      if (eventID.isEmpty()) {
        UNIOT_LOG_WARN("received event with empty eventID, ignoring");
        return;
      }

      if (valueStr.isEmpty()) {
        UNIOT_LOG_WARN("received event '%s' with empty value, ignoring", eventID.c_str());
        return;
      }

      auto value = valueStr.toInt();
      auto isNumber = value || valueStr == "0";

      if (!isNumber) {
        UNIOT_LOG_WARN("received event '%s' with non-numeric value '%s', ignoring", eventID.c_str(), valueStr.c_str());
        return;
      }

      auto sender = event.getMap("sender");
      LispEvent lispEvent;
      lispEvent.eventID = eventID;
      lispEvent.value = value;
      lispEvent.timestamp = event.getInt("timestamp");
      lispEvent.sender.type = sender.getString("type");
      lispEvent.sender.id = sender.getString("id");

      if (mEventInterceptor && !mEventInterceptor(lispEvent)) {
        // The event was not accepted by the interceptor
        return;
      }

      CoreEventListener::sendDataToChannel(unLisp::Channel::IN_EVENT, payload);
      CoreEventListener::emitEvent(unLisp::Topic::IN_LISP_EVENT, unLisp::Msg::IN_NEW_EVENT);
    }
  }

 private:
  void _populateAndPublishEvent(const Bytes &eventData) {
    CBORObject event(eventData);
    event.put("timestamp", static_cast<int64_t>(Date::now()))
        .putMap("sender")
        .put("type", "device")
        .put("id", MQTTDevice::getDeviceId().c_str());
    auto eventDataBuilt = event.build();
    auto eventID = event.getString("eventID");
    MQTTDevice::publishGroup("all", "event/" + eventID, eventDataBuilt, true);
    // NOTE: Do we need to have a separate primitive to publish not retained events?
  }

  LispEventInterceptor mEventInterceptor;
  uint32_t mChecksum;
  bool mPersist;
  bool mFailedWithError;

  String mTopicScript;
  String mTopicEvents;
};

}  // namespace uniot
