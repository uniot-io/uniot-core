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

#include <stdint.h>
#include <stddef.h>

extern "C" {
#include "c_types.h"
#include "eagle_soc.h"
#include "ets_sys.h"
#include "osapi.h"
}

namespace uniot {
  extern "C" {
    typedef struct _ETSTIMER_ ETSTimer;
  }

  class Task
  {
  public:
    using TaskCallback = void (*)(void);
    using TaskArgCallback = void (*)(void*);
  template<typename T>
    using TaskTypeCallback = void (*)(T);
    
    Task() 
    : mpTimer(nullptr) {}

    virtual ~Task() {
      detach();
    }

    void attach(uint32_t ms, bool repeat, TaskCallback callback) {
      attach_arg(ms, repeat, reinterpret_cast<TaskArgCallback>(callback), 0);
    }

  template<typename T>
    void attach(uint32_t ms, bool repeat, TaskTypeCallback<T> callback, T arg) {
      static_assert(sizeof(T) <= sizeof(uint32_t), "sizeof arg must be <= sizeof(uint32_t), i.e 4 bytes");
      attach_arg(ms, repeat, reinterpret_cast<TaskArgCallback>(callback), (uint32_t) arg);
    }

    void detach()
    {
      if (mpTimer) {
        os_timer_disarm(mpTimer);
        delete mpTimer;
        mpTimer = nullptr;
      }
    }      

    bool isAtached() {
      return mpTimer != nullptr;
    }

  private:
    ETSTimer* mpTimer;

    void attach_arg(uint32_t ms, bool repeat, TaskArgCallback callback, uint32_t arg)
    {
      if (mpTimer) {
        os_timer_disarm(mpTimer);
      } else {
        mpTimer = new ETSTimer;
      }

      os_timer_setfn(mpTimer, reinterpret_cast<ETSTimerFunc*>(callback), reinterpret_cast<void*>(arg));
      os_timer_arm(mpTimer, ms, repeat);
    }
  };
}
