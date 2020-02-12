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

#include "CoAPResource.h"

class CoAPCoreResource: public CoAPResource
{
public:
  CoAPCoreResource()
  : mResourcePath(".well-known/core", false),
  CoAPResource(&mResourcePath, COAP_METHOD_GET, COAP_CONTENTTYPE_APP_LINKFORMAT, 
    [this](coap_responsecode_t& rspCode, const String& inputBuf, String& outputBuf) {
      rspCode = COAP_RSPCODE_CONTENT;
      build(outputBuf);
    }), mpResources(nullptr) {}

  // naive algorithm so fast and economical
  void build(String& out) {
    if(mpResources) {
      // out.reserve(some_size);
      out = "";
      mpResources->begin();
      while(!mpResources->isEnd()) {
        auto resourceHigh = mpResources->next();
        auto resource = resourceHigh->getResourcePtr();
        auto contentType = COAP_GET_CONTENTTYPE(resource->content_type);
        if (contentType != COAP_CONTENTTYPE_NONE) {
          if(out.length()) {
            out += ","; // "," better than ',' due realization of String 
          }
          out += "<";
          for (size_t i = 0; i < resource->path->count; ++i) {
            out += "/";
            out += resource->path->items[i];
          }
          out += ">";
          if(resourceHigh != this) { // it is not important for ".well-known/core" resource
            out += ";ct=";
            out += static_cast<int>(contentType);
          }
        }
      }
    }
  }

  void setResources(IterableQueue<CoAPResource*>* pResources) {
    mpResources = pResources;
  }
private:
  CoAPResourcePath mResourcePath;
  IterableQueue<CoAPResource*>* mpResources;
};