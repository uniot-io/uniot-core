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

/** @cond */
/**
 * DO NOT DELETE THE "scheduling" GROUP DEFINITION BELOW.
 * Used to create the Task Scheduling topic in the documentation. If you want to delete this file,
 * please paste the group definition into another utility and delete this one.
 */
/** @endcond */

/**
 * @file TaskScheduler.h
 * @defgroup scheduling Task Scheduling
 * @brief Task scheduling system for the Uniot Core
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

#if defined(ESP8266)
using Task = ESP8266Task;
#elif defined(ESP32)
using Task = ESP32Task;
#endif

/**
 * @brief Task implementation for the Uniot scheduler system
 * @defgroup scheduler_task Task
 * @ingroup scheduling
 *
 * SchedulerTask extends the platform-specific Task implementation with
 * additional functionality for the Uniot scheduling system. It provides
 * timing control and execution tracking capabilities.
 * @{
 */
class SchedulerTask : public Task {
 public:
  /**
   * @brief Callback function signature for scheduled tasks
   *
   * @param task Reference to the executing task
   * @param times Number of repetitions left (-1 for infinite)
   */
  using SchedulerTaskCallback = std::function<void(SchedulerTask &, short)>; // TODO: add ms to callback

  /**
   * @brief Shared pointer type for task callbacks
   */
  using spSchedulerTaskCallback = SharedPointer<SchedulerTaskCallback>;

  /**
   * @brief Constructor that wraps an IExecutor implementation
   *
   * @param executor The executor to be called when the task runs
   */
  SchedulerTask(IExecutor &executor)
      : SchedulerTask([&](SchedulerTask &, short times) { executor.execute(times); }) {}

  /**
   * @brief Constructor with custom callback function
   *
   * @param callback Function to execute when the task runs
   */
  SchedulerTask(SchedulerTaskCallback callback)
      : Task(), mTotalElapsedMs(0), mRepeatTimes(0), mCanDoHardWork(false) {
    mspCallback = std::make_shared<SchedulerTaskCallback>(callback);
  }

  /**
   * @brief Attach the task to run on a specified interval
   *
   * @param ms Millisecond interval between executions
   * @param times Number of times to execute (0 or negative for infinite)
   */
  void attach(uint32_t ms, short times = 0) { // TODO: auto detach
    mRepeatTimes = times > 0 ? times : -1;
    Task::attach<volatile bool *>(ms, mRepeatTimes != 1, [](volatile bool *can) { *can = true; }, &mCanDoHardWork);
  }

  /**
   * @brief Schedule the task to run once after the specified delay
   *
   * @param ms Millisecond delay before execution
   */
  void once(uint32_t ms) {
    attach(ms, 1);
  }

  /**
   * @brief Main execution loop for the task
   *
   * Checks if the task is ready to execute and runs the callback if so.
   * Tracks execution time and manages repeat counts.
   */
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

  /**
   * @brief Get the total execution time of this task in milliseconds
   *
   * @retval time Total elapsed time in milliseconds
   */
  uint64_t getTotalElapsedMs() const {
    return mTotalElapsedMs;
  }

 private:
  uint64_t mTotalElapsedMs;     ///< Total elapsed execution time in ms
  short mRepeatTimes;           ///< Remaining repetitions (-1 for infinite)
  volatile bool mCanDoHardWork; ///< Flag indicating the task is ready to execute
  spSchedulerTaskCallback mspCallback; ///< Callback to execute
};
/** @} */

/**
 * @brief Main scheduler for managing and executing periodic tasks
 * @defgroup task_scheduler Scheduler
 * @ingroup scheduling
 *
 * TaskScheduler manages a collection of tasks, executes them according to
 * their timing requirements, and tracks performance metrics.
 */
class TaskScheduler {
  /** @addtogroup task_scheduler */
  /** @{ */
 public:
  /**
   * @brief Shared pointer type for scheduler tasks
   */
  using TaskPtr = SharedPointer<SchedulerTask>;

  /**
   * @brief Callback signature for task status reporting
   *
   * @param name Task name
   * @param isAttached Whether the task is currently attached
   * @param elapsed Total execution time in milliseconds
   */
  using TaskInfoCallback = std::function<void(const char *, bool, uint64_t)>;

  /**
   * @brief Constructor
   */
  TaskScheduler() : mTotalElapsedMs(0) {}

  /**
   * @brief Static factory method to create a task with a callback
   *
   * @param callback Function to execute when the task runs
   * @retval TaskPtr The task pointer
   */
  static TaskPtr make(SchedulerTask::SchedulerTaskCallback callback) {
    return std::make_shared<SchedulerTask>(callback);
  }

  /**
   * @brief Static factory method to create a task from an IExecutor
   *
   * @param executor The executor to wrap in a task
   * @retval TaskPtr The task pointer
   */
  static TaskPtr make(IExecutor &executor) {
    return std::make_shared<SchedulerTask>(executor);
  }

  /**
   * @brief Add a named task to the scheduler
   *
   * @param name Identifier for the task
   * @param task Shared pointer to the task
   * @retval TaskScheduler& Reference to the current scheduler instance
   */
  TaskScheduler &push(const char *name, TaskPtr task) {
    mTasks.push(MakePair(name, task));
    return *this;
  }

  /**
   * @brief Add connection kit components to the scheduler
   *
   * @param connection The connection kit to integrate
   * @retval TaskScheduler& Reference to the current scheduler instance
   */
  TaskScheduler &push(ISchedulerConnectionKit &connection) {
    connection.pushTo(*this);
    return *this;
  }

  /**
   * @brief Main execution loop for the scheduler
   *
   * Executes all registered tasks and tracks timing metrics
   */
  inline void loop() {
    auto startMs = millis();
    mTasks.forEach([&](Pair<const char *, TaskPtr> task) {
      task.second->loop();
      yield();
    });
    mTotalElapsedMs += millis() - startMs;
  }

  /**
   * @brief Report information about all registered tasks
   *
   * @param callback Function that receives information about each task
   */
  void exportTasksInfo(TaskInfoCallback callback) const {
    if (callback) {
      mTasks.forEach([&](Pair<const char *, TaskPtr> task) {
        callback(task.first, task.second->isAttached(), task.second->getTotalElapsedMs());
      });
    }
  }

  /**
   * @brief Get the total execution time of the scheduler in milliseconds
   *
   * @retval time Total elapsed time in milliseconds
   */
  uint64_t getTotalElapsedMs() const {
    return mTotalElapsedMs;
  }

 private:
  uint64_t mTotalElapsedMs; ///< Total elapsed scheduler execution time in ms
  ClearQueue<Pair<const char *, TaskPtr>> mTasks; ///< Collection of managed tasks
  /** @} */
};

}  // namespace uniot
