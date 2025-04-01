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
/**
 * @brief Interface for executing tasks in the scheduler system
 *
 * IExecutor provides a common interface for any class that needs to be
 * executed by the TaskScheduler. This enables different components to
 * be integrated into the scheduling system.
 */
class IExecutor
{
public:
  /**
   * @brief Virtual destructor for proper cleanup
   */
  virtual ~IExecutor() {}

  /**
   * @brief Execute the implementation's functionality
   *
   * @param times The number of remaining executions:
   *        - Positive numbers indicate remaining executions count
   *        - Negative numbers indicate infinite executions (repeat forever)
   *        - Zero indicates the last execution has occurred
   */
  virtual void execute(short times) = 0;
};
} // namespace uniot
