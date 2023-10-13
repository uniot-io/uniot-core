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

#include <unLisp.h>
#include <MQTTDevice.h>
#include <CBORStorage.h>
#include <EventListener.h>

namespace uniot
{

class LispDevice : public MQTTDevice, public CBORStorage, public CoreEventListener
{
public:
  LispDevice() : MQTTDevice(), CBORStorage("lisp.cbor"), CoreEventListener(),
                 mFirstPacketReceived(false)
  {
    CoreEventListener::listenToEvent(unLisp::LISP);
  }

  void runStoredCode()
  {
    if (CBORStorage::restore())
    {
      auto code = object().getString("code");
      auto persist = object().getInt("persist");
      mChecksum = object().getInt("checksum");

      if (persist && code.length() > 0)
        getLisp().runCode(code);
    }
  }

  unLisp &getLisp()
  {
    return unLisp::getInstance();
  }

  bool store()
  {
    mChecksum = getLisp().getLastCode().checksum();
    object()
        .put("persist", getLisp().isLastCodePersist())
        .put("checksum", (int) mChecksum);

    if (getLisp().isLastCodePersist())
      object()
          .put("code", getLisp().getLastCode().c_str())
          .forceDirty(); // Avoid optimization. The pointer to the data may be the same. Read the warnings.
    return CBORStorage::store();
  }

  void onEventReceived(unsigned int topic, int msg) override
  {
    if (msg == unLisp::ERROR)
    {
      auto lastError = getLisp().getLastError();
      publishDevice("script_error", lastError);
      UNIOT_LOG_ERROR("lisp error: %s", lastError.c_str());
    }
    else if (msg == unLisp::MSG_ADDED)
    {
      auto size = getLisp().sizeOutput();
      auto result = getLisp().popOutput();
      UNIOT_LOG_INFO("%s, %d msgs to read; %s", msg == unLisp::MSG_ADDED ? "added" : "replaced", size, result.c_str());
    }
  }

  void handle(const String &topic, const Bytes &payload) override
  {
    auto packet = CBORObject(payload);
    auto script = Bytes(packet.getString("code"));

    auto newChecksum = script.terminate().checksum();
    auto isEqual = mChecksum == newChecksum;
    auto ignoreScript = isEqual;

    if (mFirstPacketReceived)
      ignoreScript = isEqual && getLisp().isLastCodePersist();
    else
      mFirstPacketReceived = true;

    if (!ignoreScript)
    {
      // TODO: check signature here later
      getLisp().runCode(script);

      if (!isEqual)
        LispDevice::store();
    }
    else
      UNIOT_LOG_INFO("script ignored: %s", script.c_str());

    // let's free some memory
    object().clean();
    // NOTE: you may need getLasCode() somewhere else, but it has already cleaned here
    getLisp().cleanLastCode();
  }

private:
  bool mFirstPacketReceived;
  uint32_t mChecksum;
};

} // namespace uniot
