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

/**
 * @file Logger.h
 * @defgroup logger Logger
 * @brief Logging system for Uniot devices
 * @{
 *
 * This file provides a customizable logging system for Uniot IoT devices.
 * It defines macros for different log levels and provides a consistent
 * logging interface that can be conditionally compiled based on settings.
 *
 * The logger outputs messages with timestamps, file info, and function names
 * to help with debugging and monitoring device operations.
 */

#pragma once

/**
 * @def __FUNC_NAME__
 * @brief Cross-platform function name macro
 *
 * Defines a cross-compiler compatible way to get the current function name
 * Works with GCC, Clang, and other compilers
 */
#ifndef __FUNC_NAME__
#if defined(__GNUC__) || defined(__clang__)
  #define __FUNC_NAME__ __func__
#else
  #define __FUNC_NAME__ __FUNCTION__
#endif
#endif

/**
 * @brief Log level definitions
 *
 * Defines the various logging levels in order of verbosity:
 * - ERROR: Critical errors that prevent normal operation
 * - WARN:  Warnings that don't stop operation but indicate issues
 * - INFO:  General information about system operation
 * - DEBUG: Detailed information for debugging purposes
 * - TRACE: Very detailed tracing information for deep debugging
 */
#define UNIOT_LOG_LEVEL_ERROR 0
#define UNIOT_LOG_LEVEL_WARN  1
#define UNIOT_LOG_LEVEL_INFO  2
#define UNIOT_LOG_LEVEL_DEBUG 3
#define UNIOT_LOG_LEVEL_TRACE 4

/**
 * @brief Default log level
 *
 * Sets the default logging level if not defined elsewhere.
 * Can be overridden by defining UNIOT_LOG_LEVEL before including this file.
 */
#ifndef UNIOT_LOG_LEVEL
#define UNIOT_LOG_LEVEL UNIOT_LOG_LEVEL_DEBUG
#endif

/**
 * @brief Logging implementation when enabled
 *
 * When UNIOT_LOG_ENABLED is defined, this section provides the actual
 * implementation of the logging functionality.
 */
#if UNIOT_LOG_ENABLED

#include <Arduino.h>

/**
 * @brief Default serial port for logging
 *
 * Can be overridden by defining UNIOT_LOG_STREAM before including this file.
 */
#ifndef UNIOT_LOG_STREAM
#define UNIOT_LOG_STREAM Serial
#endif

/**
 * @brief Default baud rate for serial logging
 *
 * Sets the communication speed for the logging serial port.
 * Can be overridden by defining UNIOT_LOG_BAUD_RATE before including this file.
 */
#ifndef UNIOT_LOG_BAUD_RATE
#define UNIOT_LOG_BAUD_RATE 115200
#endif

/**
 * @brief Maximum size of the logging buffer
 *
 * Defines the maximum size of a single log message.
 * Messages longer than this will be truncated with "[...]" indicator.
 */
#ifndef UNIOT_LOG_BUF_SIZE
#define UNIOT_LOG_BUF_SIZE 256
#endif

/**
 * @brief Initialize the logging system
 *
 * Sets up the serial port for logging if not already initialized.
 * Can be overridden by defining UNIOT_LOG_SET_READY before including this file.
 */
#ifndef UNIOT_LOG_SET_READY
#define UNIOT_LOG_SET_READY()          \
  do {                                 \
    if (!UNIOT_LOG_STREAM) {                 \
      UNIOT_LOG_STREAM.begin(UNIOT_LOG_BAUD_RATE); \
      UNIOT_LOG_STREAM.print("\n\n");        \
    }                                  \
  } while (0)
#endif

/**
 * @brief Print to the logging stream
 *
 * Outputs text to the configured logging stream if available.
 * Can be overridden by defining UNIOT_LOG_PRINT before including this file.
 */
#ifndef UNIOT_LOG_PRINT
#define UNIOT_LOG_PRINT(...)         \
  do {                               \
    if (UNIOT_LOG_STREAM)                  \
      UNIOT_LOG_STREAM.print(__VA_ARGS__); \
  } while (0)
#endif

/**
 * @brief Core logging macro
 *
 * Formats and outputs a log message with:
 * - Log level indicator
 * - Timestamp (milliseconds)
 * - File name and line number
 * - Function name
 * - User message with printf-style formatting
 *
 * @param log_type Type/level of the log message (ERROR, WARN, etc.)
 * @param log_fmt Format string for the message
 * @param log_arg Variable arguments for format string
 */
#define UNIOT_LOG(log_type, log_fmt, log_arg...)                         \
  do {                                                                   \
    uniot_log_printf("[" #log_type "][%lu][%s:%d][%s] " log_fmt "\n",    \
                     millis(), __FILE__, __LINE__, __FUNC_NAME__, ##log_arg); \
  } while (0)

/**
 * @brief Conditional logging macro
 *
 * Only outputs a log message if the specified condition is true.
 *
 * @param log_type Type/level of the log message
 * @param log_cond Condition that must be true for the message to be logged
 * @param log_fmt Format string for the message
 * @param log_arg Variable arguments for format string
 */
#define UNIOT_LOG_IF(log_type, log_cond, log_fmt, log_arg...) \
  do {                                                        \
    if (log_cond)                                             \
      UNIOT_LOG(log_type, log_fmt, ##log_arg);                \
  } while (0)

/**
 * @brief Printf-style formatting function for log messages
 *
 * Formats a log message using vsnprintf and outputs it to the logging stream.
 * Handles buffer overflow by adding a truncation indicator.
 *
 * @param format Printf-style format string
 * @param ... Variable arguments for the format string
 * @retval int Length of the formatted string
 */
static inline int
uniot_log_printf(const char *format, ...) {
  va_list arg;
  va_start(arg, format);
  char buf[UNIOT_LOG_BUF_SIZE];
  int len = vsnprintf(buf, sizeof(buf), format, arg);
  va_end(arg);
  UNIOT_LOG_PRINT(buf);
  if (len >= (int)sizeof(buf))
    UNIOT_LOG_PRINT(" [...]\n");
  return len;
}

#else  /* UNIOT_LOG_ENABLED not defined */
/**
 * @brief Stub implementations when logging is disabled
 *
 * When logging is disabled, these no-op macros ensure that
 * log calls don't generate any code while still being syntactically valid.
 */
#include <Common.h>

#define UNIOT_LOG_SET_READY() do {} while(0)
#define UNIOT_LOG_PRINT(...) (UNUSED(__VA_ARGS__))
#define UNIOT_LOG(log_type, ...) (UNUSED(__VA_ARGS__))
#define UNIOT_LOG_IF(log_type, ...) (UNUSED(__VA_ARGS__))
#endif

#if UNIOT_LOG_LEVEL_ERROR <= UNIOT_LOG_LEVEL
/**
 * @brief Log an ERROR level message
 * Used for critical errors that may prevent normal operation.
 * Only compiled if UNIOT_LOG_LEVEL is at least UNIOT_LOG_LEVEL_ERROR.
 * @param ... Printf-style format string followed by arguments
 */
#define UNIOT_LOG_ERROR(...) UNIOT_LOG(ERROR, __VA_ARGS__)
/**
 * @brief Conditionally log an ERROR level message
 * Used for critical errors that may prevent normal operation.
 * Only compiled if UNIOT_LOG_LEVEL is at least UNIOT_LOG_LEVEL_ERROR.
 * @param log_cond Condition that must be true for the message to be logged
 * @param log... Printf-style format string followed by arguments
 */
#define UNIOT_LOG_ERROR_IF(log_cond, log...) UNIOT_LOG_IF(ERROR, log_cond, ##log)
#else
#define UNIOT_LOG_ERROR(...) (UNUSED(__VA_ARGS__))
#define UNIOT_LOG_ERROR_IF(...) (UNUSED(__VA_ARGS__))
#endif

#if UNIOT_LOG_LEVEL_WARN <= UNIOT_LOG_LEVEL
/**
 * @brief Log an WARN level message
 * Used for warnings about potentially problematic situations.
 * Only compiled if UNIOT_LOG_LEVEL is at least UNIOT_LOG_LEVEL_WARN.
 * @param ... Printf-style format string followed by arguments
 */
#define UNIOT_LOG_WARN(...) UNIOT_LOG(WARN, __VA_ARGS__)
/**
 * @brief Conditionally log an WARN level message
 * Used for warnings about potentially problematic situations.
 * Only compiled if UNIOT_LOG_LEVEL is at least UNIOT_LOG_LEVEL_WARN.
 * @param log_cond Condition that must be true for the message to be logged
 * @param log... Printf-style format string followed by arguments
 */
#define UNIOT_LOG_WARN_IF(log_cond, log...) UNIOT_LOG_IF(WARN, log_cond, ##log)
#else
#define UNIOT_LOG_WARN(...) (UNUSED(__VA_ARGS__))
#define UNIOT_LOG_WARN_IF(...) (UNUSED(__VA_ARGS__))
#endif

#if UNIOT_LOG_LEVEL_INFO <= UNIOT_LOG_LEVEL
/**
 * @brief Log an INFO level message
 * Used for general information about system operation.
 * Only compiled if UNIOT_LOG_LEVEL is at least UNIOT_LOG_LEVEL_INFO.
 * @param ... Printf-style format string followed by arguments
 */
#define UNIOT_LOG_INFO(...) UNIOT_LOG(INFO, __VA_ARGS__)
/**
 * @brief Conditionally log an INFO level message
 * Used for general information about system operation.
 * Only compiled if UNIOT_LOG_LEVEL is at least UNIOT_LOG_LEVEL_INFO.
 * @param log_cond Condition that must be true for the message to be logged
 * @param log... Printf-style format string followed by arguments
 */
#define UNIOT_LOG_INFO_IF(log_cond, log...) UNIOT_LOG_IF(INFO, log_cond, ##log)
#else
/**
 * @brief Stub implementation when INFO logging is disabled
 * Uses UNUSED macro to prevent unused variable warnings
 */
#define UNIOT_LOG_INFO(...) (UNUSED(__VA_ARGS__))
#define UNIOT_LOG_INFO_IF(...) (UNUSED(__VA_ARGS__))
#endif

#if UNIOT_LOG_LEVEL_DEBUG <= UNIOT_LOG_LEVEL
/**
 * @brief Log an DEBUG level message
 * Used for general information about system operation.
 * Only compiled if UNIOT_LOG_LEVEL is at least UNIOT_LOG_LEVEL_DEBUG.
 * @param ... Printf-style format string followed by arguments
 */
#define UNIOT_LOG_DEBUG(...) UNIOT_LOG(DEBUG, __VA_ARGS__)
/**
 * @brief Conditionally log an DEBUG level message
 * Used for general information about system operation.
 * Only compiled if UNIOT_LOG_LEVEL is at least UNIOT_LOG_LEVEL_DEBUG.
 * @param log_cond Condition that must be true for the message to be logged
 * @param log... Printf-style format string followed by arguments
 */
#define UNIOT_LOG_DEBUG_IF(log_cond, log...) UNIOT_LOG_IF(DEBUG, log_cond, ##log)
#else
#define UNIOT_LOG_DEBUG(...) (UNUSED(__VA_ARGS__))
#define UNIOT_LOG_DEBUG_IF(...) (UNUSED(__VA_ARGS__))
#endif

#if UNIOT_LOG_LEVEL_TRACE <= UNIOT_LOG_LEVEL
/**
 * @brief Log an TRACE level message
 * Used for general information about system operation.
 * Only compiled if UNIOT_LOG_LEVEL is at least UNIOT_LOG_LEVEL_TRACE.
 * @param ... Printf-style format string followed by arguments
 */
#define UNIOT_LOG_TRACE(...) UNIOT_LOG(TRACE, __VA_ARGS__)
/**
 * @brief Conditionally log an TRACE level message
 * Used for general information about system operation.
 * Only compiled if UNIOT_LOG_LEVEL is at least UNIOT_LOG_LEVEL_TRACE.
 * @param log_cond Condition that must be true for the message to be logged
 * @param log... Printf-style format string followed by arguments
 */
#define UNIOT_LOG_TRACE_IF(log_cond, log...) UNIOT_LOG_IF(TRACE, log_cond, ##log)
#else
#define UNIOT_LOG_TRACE(...) (UNUSED(__VA_ARGS__))
#define UNIOT_LOG_TRACE_IF(...) (UNUSED(__VA_ARGS__))
#endif

/**
 * @}
 * End of logger group
 */
