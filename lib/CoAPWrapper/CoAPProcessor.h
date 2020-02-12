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

#include <coap.h>
#include <IterableQueue.h>
#include "CoAPResource.h"
#include "CoAPCoreResource.h"
#include "CoAPRequest.h"

class CoAPProcessor
{
public:
  using ResponseHandler = std::function<void(const uint16_t msgid, const String& inputBuf)>;
  
  CoAPProcessor() : mResponseHandler(nullptr) {
    mResources.push(&mCoreResource);
    mCoreResource.setResources(&mResources);
  }

  void addResource(CoAPResource* pResource) {
    mResources.push(pResource);
  }

  void setResponseHandler(ResponseHandler handler) {
    mResponseHandler = handler;
  }

  bool handleRequest(const coap_packet_t* inputPacket, coap_packet_t* outputPacket) {
    uint8_t optionsCount;
    auto responseCode = COAP_RSPCODE_NOT_IMPLEMENTED;
    auto options = coap_find_uri_path(inputPacket, &optionsCount);
    if(options) {
      mResources.begin();
      while(!mResources.isEnd()) {
        auto resourceHigh = mResources.next();
        auto resource = resourceHigh->getResourcePtr();
        if(resource->method == inputPacket->hdr.code) {  // TODO: need to swap these two conditions ?
          if(coap_check_resource(resource, options, optionsCount) == COAP_ERR_NONE) {
            // FOUND
            responseCode = COAP_RSPCODE_CONTENT;
            if(resourceHigh->mHandler) {
              resourceHigh->mHandler(responseCode, String((const char*)inputPacket->payload.p), mOutputStringBuffer);
              return (COAP_ERR_MAX < coap_make_response(inputPacket->hdr.id, &inputPacket->tok, 
                COAP_TYPE_ACK, responseCode, resource->content_type,
                (uint8_t*)mOutputStringBuffer.c_str(), mOutputStringBuffer.length(), outputPacket));
            }
          } else {
            responseCode = COAP_RSPCODE_NOT_FOUND;
          }
        } else {
          responseCode = COAP_RSPCODE_METHOD_NOT_ALLOWED;
        }
      }
    }
    // NOT FOUND
    return (COAP_ERR_MAX < coap_make_response(inputPacket->hdr.id, &inputPacket->tok, 
      COAP_TYPE_ACK, responseCode, NULL, 
      NULL, 0, outputPacket));
  }

  bool handleResponse(const coap_packet_t* inputPacket) {
    auto code = inputPacket->hdr.code;
    if (!(code >= 0 && code <= 4)) { // NOT: Empty, Get, Post, Put, Delete
      if(mResponseHandler) {
        mResponseHandler(inputPacket->hdr.id, String((const char*)inputPacket->payload.p));
      }
      return true;
    }
    return false;
  }

  uint16_t makeRequest(CoAPRequest* request, coap_packet_t* outputPacket) {
    // do not worry about local variable
    auto resource = coap_make_request_resource(request->mMethod, request->mpResourcePath);
    auto msgId = request->getNewMsgId();
    auto outCode = coap_make_request(msgId, NULL, &resource,
      (const uint8_t*)request->mPayload.c_str(), request->mPayload.length(), outputPacket);
    return (COAP_ERR_MAX < outCode) ? msgId : 0;
  }

  void flush() {
    mOutputStringBuffer = (const char*) NULL;
  }

private:
  CoAPCoreResource mCoreResource;
  IterableQueue<CoAPResource*> mResources;
  String mOutputStringBuffer;
  ResponseHandler mResponseHandler;
};