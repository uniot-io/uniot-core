/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2020 Uniot <info.uniot@gmail.com>
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

#include <CoAPKit.h>

class CoAPRelay
{
public:
  CoAPRelay(uint8_t pin, const char* path)
  : mPin(pin), 
  mPath(path), 
  mGetResource(&mPath, COAP_METHOD_GET, COAP_CONTENTTYPE_TXT_PLAIN, 
    [this](coap_responsecode_t& rspCode, const String& inputBuf, String& outputBuf) {
      outputBuf = this->getState();
    }),
  mPostResource(&mPath, COAP_METHOD_POST, COAP_CONTENTTYPE_NONE, 
    [this](coap_responsecode_t& rspCode, const String& inputBuf, String& outputBuf) {
      setState(inputBuf.toInt());
      outputBuf = getState();
    }) 
  { 
    pinMode(mPin, OUTPUT); 
  }

  bool getState() {
    return digitalRead(mPin);
  }

  void setState(bool state) {
    digitalWrite(mPin, state);
  }

  void connect(CoAPKit* coap) {
    if(coap) {
      coap->addResource(&mGetResource);
      coap->addResource(&mPostResource);
    }
  }

private:
  uint8_t mPin;
  CoAPResourcePath mPath;
  CoAPResource mGetResource;
  CoAPResource mPostResource;
};