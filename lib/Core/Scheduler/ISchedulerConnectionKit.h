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

namespace uniot
{
class TaskScheduler;

/**
 * @brief Interface for connecting components to the TaskScheduler
 * @ingroup scheduling
 * @{
 *
 * This interface facilitates the integration of different components
 * with the TaskScheduler. Implementing classes can register themselves
 * and their tasks with the scheduler system.
 */
class ISchedulerConnectionKit
{
public:
  /**
   * @brief Virtual destructor for proper cleanup
   */
  virtual ~ISchedulerConnectionKit() {}

  /**
   * @brief Register this component with the given scheduler
   *
   * Implementing classes use this method to add themselves or their
   * tasks to the provided scheduler instance.
   *
   * @param scheduler The scheduler to connect with
   */
  virtual void pushTo(TaskScheduler &scheduler) = 0;

  /**
   * @brief Initialize and activate the component
   *
   * This method should prepare the component for use, typically called
   * after the component has been registered with a scheduler.
   */
  virtual void attach() = 0;
};
/** @} */
} // namespace uniot
