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

#include <IExecutor.h>
#include <memory>

#include "CoAPResource.h"
#include "CoAPProcessor.h"
#include "CoAPCommunicator.h"
//#include <coap_dump.h>

class CoAPKit : public uniot::IExecutor
{
public:
  CoAPKit(UDP* udp)
  : mRequested(false),
  mBroadcastRandomFactor(10),
  mpCommunicator(new CoAPCommunicator(udp)), 
  mpProcessor(new CoAPProcessor()) { }

  void addResource(CoAPResource* pResource) {
    mpProcessor->addResource(pResource);
  }

  bool begin(IPAddress subnetMask) {
    return mpCommunicator->begin(subnetMask);
  }

  void stop() {
    mpCommunicator->stop();
  }

  void setBroadcastRandomFactor(uint8_t value) {
    mBroadcastRandomFactor = value;
  }

  uint16_t doRequest(IPAddress address, CoAPRequest* request) {
    mRemoteAddress = address;
    auto msgId = mpProcessor->makeRequest(request, mpCommunicator->getOutputPacketPtr());
    mRequested = msgId;
    return msgId;
  }

  bool canDoRequestNow() {
    return !mRequested;
  }

  void setResponseHandler(CoAPProcessor::ResponseHandler handler) {
    mpProcessor.get()->setResponseHandler(handler);
  }

  virtual uint8_t execute() override {
    static uint8_t responseDelayCounter = 0;
    if(mRequested) {
      mpCommunicator->sendRequest(mRemoteAddress);
      mRequested = false;
      responseDelayCounter = 0;   // because we broke the OutputPacket if request has been received and processed
    } else {
      if(mpCommunicator->isRunned() && mpCommunicator->receive()) {
        // coap_dump_packet(mpCommunicator->getInputPacketPtr());
        // expect until the packet is sent
        if(!responseDelayCounter && !mpProcessor->handleResponse(mpCommunicator->getInputPacketPtr())) {
          mpProcessor->handleRequest(mpCommunicator->getInputPacketPtr(), mpCommunicator->getOutputPacketPtr());
          responseDelayCounter = mpCommunicator->isLastBroadcast() ? random(2, mBroadcastRandomFactor) : 1;
        }
      }
      if(responseDelayCounter > 0 && !(--responseDelayCounter)) {
        mpCommunicator->sendResponse();
        // coap_dump_packet(mpCommunicator->getOutputPacketPtr());
        mpProcessor->flush();
      }
    }
  }

private:
  bool mRequested;
  uint8_t mBroadcastRandomFactor;
  IPAddress mRemoteAddress;
  std::unique_ptr<CoAPCommunicator> mpCommunicator;
  std::unique_ptr<CoAPProcessor> mpProcessor;
};