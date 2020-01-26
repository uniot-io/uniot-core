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

#include <functional>
#include <IExecutor.h>
#include <ISchedulerKitConnection.h>
#include <ClearQueue.h>
#include <memory>
#include "Task.h"

namespace uniot
{
class SchedulerTask : public Task, public IExecutor
{
public:
  // TODO: add ms to callback
  using SchedulerTaskCallback = std::function<void(short)>;
  using spSchedulerTaskCallback = std::shared_ptr<SchedulerTaskCallback>;

  SchedulerTask(IExecutor *executor)
      : SchedulerTask([=](short) { executor->execute(); }) {}

  SchedulerTask(SchedulerTaskCallback callback)
      : Task(), mRepeatTimes(0), mCanDoHardWork(false)
  {
    mspCallback = std::make_shared<SchedulerTaskCallback>(callback);
  }

  //TODO: auto detach, maybe mCanDoHardWork should be countable type, if mCanDoHardWork > x then detach, refresh when executed
  void attach(uint32_t ms, short times = 0)
  {
    mRepeatTimes = times > 0 ? times : -1;
    Task::attach<volatile bool *>(ms, mRepeatTimes != 1, [](volatile bool *can) { *can = true; }, &mCanDoHardWork);
  }

  virtual uint8_t execute() override
  {
    if (mCanDoHardWork)
    {
      mCanDoHardWork = false;

      if (mRepeatTimes > 0 && !--mRepeatTimes)
      {
        Task::detach();
      }
      (*mspCallback)(mRepeatTimes);
    }
    return isAtached();
  }

private:
  short mRepeatTimes;
  volatile bool mCanDoHardWork;
  spSchedulerTaskCallback mspCallback;
};

class TaskScheduler : public IExecutor
{
public:
  using TaskPtr = std::shared_ptr<SchedulerTask>;

  static TaskPtr make(SchedulerTask::SchedulerTaskCallback callback)
  {
    return std::make_shared<SchedulerTask>(callback);
  }

  static TaskPtr make(IExecutor *executor)
  {
    return std::make_shared<SchedulerTask>(executor);
  }

  TaskScheduler *push(TaskPtr task)
  {
    mTasks.push(task);
    return this;
  }

  TaskScheduler *push(ISchedulerKitConnection *connection)
  {
    connection->pushTo(this);
    return this;
  }

  virtual uint8_t execute() override
  {
    byte attached = 0;
    mTasks.forEach([&](TaskPtr task) { attached += task->execute(); yield(); });
    return attached;
  }

private:
  ClearQueue<TaskPtr> mTasks;
};
} // namespace uniot
