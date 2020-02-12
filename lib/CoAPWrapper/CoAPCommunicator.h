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

#include <Udp.h>
#include <coap.h>

#ifndef COAP_BUFFER_SIZE
#define COAP_BUFFER_SIZE 256 // UDP_TX_PACKET_MAX_SIZE to large for me :)
#endif

class CoAPCommunicator
{
public:
  CoAPCommunicator(UDP* udp)
  : mpUDP(udp),
  mIsRunned(false),
  mIsLastBroadcast(false) { }

  bool begin(IPAddress subnetMask) {
    mSubnetMask = subnetMask;
    if(isRunned()) {
      stop();
    }
    return mIsRunned = mpUDP->begin(COAP_DEFAULT_PORT);
  }

  void stop() {
    mpUDP->stop();
    mIsRunned = false;
  }

  bool isRunned() {
    return mIsRunned;
  }

  bool isLastBroadcast() {
    return mIsLastBroadcast;
  }

  bool receive() {
    size_t receivedSize = mpUDP->parsePacket();
    if (receivedSize) {
      memset(mByteBuffer, 0, sizeof(mByteBuffer));
      mpUDP->read(mByteBuffer, sizeof(mByteBuffer));
      mpUDP->flush();
      if (coap_parse(mByteBuffer, receivedSize, &mInputPacket) == COAP_ERR_NONE) {
        mSavedRemoteIP = mpUDP->remoteIP();
        mSavedRemotePort = mpUDP->remotePort();
#ifdef ESP8266
        mIsLastBroadcast = _isBroadcast(((WiFiUDP*)mpUDP)->destinationIP());
#endif
        return true;
      }
    }
    return false;
  }

  bool send(const IPAddress& address, uint16_t port) {
    // memset(mByteBuffer, 0, sizeof(mByteBuffer)); // TODO: Is it necessary?
    auto bufSize = sizeof(mByteBuffer);
    if (coap_build(&mOutputPacket, mByteBuffer, &bufSize) == COAP_ERR_NONE) {
      _udpSend(address, port, mByteBuffer, bufSize);
      return true;
    }
    return false;
  }

  bool sendResponse() {
    return send(mSavedRemoteIP, mSavedRemotePort);
  }

  bool sendRequest(IPAddress address) {
    _udpCheatBeforeSend(address, COAP_DEFAULT_PORT);
    return send(address, COAP_DEFAULT_PORT);
  }

  coap_packet_t* getInputPacketPtr() {
    return &mInputPacket;
  }

  coap_packet_t* getOutputPacketPtr() {
    return &mOutputPacket;
  }

private:
  void _udpCheatBeforeSend(const IPAddress& address, uint16_t port) {
    mpUDP->beginPacket(address, port);
    mpUDP->endPacket();
  }

  void _udpSend(const IPAddress& address, uint16_t port, const uint8_t *buf, int buflen) {
    mpUDP->beginPacket(address, port);
    mpUDP->write(buf, buflen);
    mpUDP->endPacket();
  }

  bool _isBroadcast(const IPAddress& address) {
    return address == (~mSubnetMask | address);
  }

// FIXME: if swap places of mByteBuffer and mIsRunned it crashes. Why?
  uint8_t mByteBuffer[COAP_BUFFER_SIZE];
  coap_packet_t mInputPacket;
  coap_packet_t mOutputPacket;
  IPAddress mSubnetMask;
  IPAddress mSavedRemoteIP;
  uint16_t mSavedRemotePort;
  bool mIsLastBroadcast;
  bool mIsRunned;
  UDP* mpUDP;
};