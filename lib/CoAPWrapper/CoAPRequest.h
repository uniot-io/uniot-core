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

class CoAPRequest
{
public:
  friend class CoAPProcessor;

  CoAPRequest(CoAPResourcePath* path, coap_method_t method, const String& payload = "")
  : mpResourcePath(path->getResourcePathPtr()),
  mMethod(method),
  mPayload(payload) { }

  void flush() {
    mPayload = (const char*) NULL;
  }

protected:
  static uint16_t getNewMsgId() {
    return ++MESSAGE_COUNTER;
  }

private:
  static uint16_t MESSAGE_COUNTER;

  coap_method_t mMethod;
  String mPayload;
  coap_resource_path_t* mpResourcePath;
};

uint16_t CoAPRequest::MESSAGE_COUNTER = 0;