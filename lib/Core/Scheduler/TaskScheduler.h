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

#if defined(ESP8266)
#include "ESP8266Task.h"
#elif defined(ESP32)
#include "ESP32Task.h"
#endif

#include <ClearQueue.h>
#include <Common.h>
#include <IExecutor.h>
#include <ISchedulerConnectionKit.h>

#include <functional>
#include <memory>

namespace uniot {
class SchedulerTask : public Task {
 public:
  // TODO: add ms to callback
  using SchedulerTaskCallback = std::function<void(SchedulerTask &, short)>;
  using spSchedulerTaskCallback = SharedPointer<SchedulerTaskCallback>;

  SchedulerTask(IExecutor &executor)
      : SchedulerTask([&](SchedulerTask &, short) { executor.execute(); }) {}

  SchedulerTask(SchedulerTaskCallback callback)
      : Task(), mTotalElapsedMs(0), mRepeatTimes(0), mCanDoHardWork(false) {
    mspCallback = std::make_shared<SchedulerTaskCallback>(callback);
  }

  // TODO: auto detach, maybe mCanDoHardWork should be countable type, if mCanDoHardWork > x then detach, refresh when executed
  void attach(uint32_t ms, short times = 0) {
    mRepeatTimes = times > 0 ? times : -1;
    Task::attach<volatile bool *>(ms, mRepeatTimes != 1, [](volatile bool *can) { *can = true; }, &mCanDoHardWork);
  }

  void once(uint32_t ms) {
    attach(ms, 1);
  }

  inline void loop() {
    auto startMs = millis();
    if (mCanDoHardWork) {
      mCanDoHardWork = false;

      if (mRepeatTimes > 0 && !--mRepeatTimes) {
        Task::detach();
      }
      (*mspCallback)(*this, mRepeatTimes);
    }
    mTotalElapsedMs += millis() - startMs;
  }

  uint64_t getTotalElapsedMs() const {
    return mTotalElapsedMs;
  }

 private:
  uint64_t mTotalElapsedMs;
  short mRepeatTimes;
  volatile bool mCanDoHardWork;
  spSchedulerTaskCallback mspCallback;
};

class TaskScheduler {
 public:
  using TaskPtr = SharedPointer<SchedulerTask>;
  using TaskInfoCallback = std::function<void(const char *, bool, uint64_t)>;

  TaskScheduler() : mTotalElapsedMs(0) {}

  static TaskPtr make(SchedulerTask::SchedulerTaskCallback callback) {
    return std::make_shared<SchedulerTask>(callback);
  }

  static TaskPtr make(IExecutor &executor) {
    return std::make_shared<SchedulerTask>(executor);
  }

  TaskScheduler &push(const char *name, TaskPtr task) {
    mTasks.push(MakePair(name, task));
    return *this;
  }

  TaskScheduler &push(ISchedulerConnectionKit &connection) {
    connection.pushTo(*this);
    return *this;
  }

  inline void loop() {
    auto startMs = millis();
    mTasks.forEach([&](Pair<const char *, TaskPtr> task) {
      task.second->loop();
      yield();
    });
    mTotalElapsedMs += millis() - startMs;
  }

  void exportTasksInfo(TaskInfoCallback callback) const {
    if (callback) {
      mTasks.forEach([&](Pair<const char *, TaskPtr> task) {
        callback(task.first, task.second->isAttached(), task.second->getTotalElapsedMs());
      });
    }
  }

  uint64_t getTotalElapsedMs() const {
    return mTotalElapsedMs;
  }

 private:
  uint64_t mTotalElapsedMs;
  ClearQueue<Pair<const char *, TaskPtr>> mTasks;
};
}  // namespace uniot
