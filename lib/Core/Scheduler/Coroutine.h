/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2024 Uniot <contact@uniot.io>
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

/**
 * @file Coroutine.h
 * @brief Lightweight cooperative multitasking through coroutines.
 *
 * This file implements a simple, stackless coroutine mechanism using macros.
 * Coroutines allow functions to suspend their execution at certain points
 * and resume later from where they left off, which is useful for cooperative
 * multitasking, asynchronous operations, and state machines.
 *
 * Usage example:
 *
 * ```cpp
 * void myCoroutine() {
 *   COROUTINE_BEGIN();
 *
 *   // Do something
 *   doTask1();
 *   COROUTINE_YIELD();
 *
 *   // When resumed, continue from here
 *   doTask2();
 *   COROUTINE_YIELD();
 *
 *   doFinalTask();
 *   COROUTINE_END();
 * }
 * ```
 */

/**
 * @brief Begins a coroutine function block.
 *
 * This macro must be placed at the beginning of any function that implements
 * a coroutine. It initializes the coroutine state machine by creating a static
 * variable to track execution position and setting up the switch statement
 * that allows returning to the correct point after yielding.
 *
 * The static state variable preserves the coroutine's position between calls.
 */
#define COROUTINE_BEGIN()         \
  static uint32_t coroutine_state = 0; \
  switch (coroutine_state) {      \
    case 0:

/**
 * @brief Suspends the coroutine execution and returns control.
 *
 * When a coroutine yields, it saves its current line number as the state
 * and returns from the function. The next time the coroutine is called,
 * execution resumes right after this yield point.
 *
 * After resuming, the state is reset to 0 to ensure proper flow if the
 * coroutine completes and needs to be restarted.
 */
#define COROUTINE_YIELD()       \
  do {                          \
    coroutine_state = __LINE__; \
    return;                     \
    case __LINE__:;             \
      coroutine_state = 0;      \
  } while (0)

/**
 * @brief Marks the end of a coroutine function.
 *
 * This macro closes the switch statement opened by COROUTINE_BEGIN() and
 * performs a final return. After this point, the coroutine has completed
 * its execution cycle. When called again, the coroutine will start from
 * the beginning (as the state would be 0).
 */
#define COROUTINE_END() \
  }                     \
  return
