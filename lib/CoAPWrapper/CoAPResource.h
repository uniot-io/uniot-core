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

#include <coap_ext.h>
#include <functional>
#include "CoAPResourcePath.h"

class CoAPResource
{
public:
  friend class CoAPProcessor;
  using Handler = std::function<void(coap_responsecode_t& rspCode, const String& inputBuf, String& outputBuf)>;
  
  // be careful "path" will be splitted by strtok()
  CoAPResource(CoAPResourcePath* path, coap_method_t method, coap_content_type_t contentType, Handler handler) : mHandler(handler) {
    coap_set_content_type(&mResource, contentType);
    mResource.path = path->getResourcePathPtr();
    mResource.method = method;
  }

  coap_resource_ext_t* getResourcePtr() {
    return &mResource;
  }

  virtual ~CoAPResource() {}
private:
  Handler mHandler;
  coap_resource_ext_t mResource;
};