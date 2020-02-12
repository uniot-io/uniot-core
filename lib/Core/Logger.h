#pragma once

#if UNIOT_LOG_ENABLED

#include <Arduino.h>

#ifndef UNIOT_LOG_BUF_SIZE
#define UNIOT_LOG_BUF_SIZE 128
#endif

#ifndef UNIOT_LOG_PRINT
#define UNIOT_LOG_PRINT(...)   \
  do                           \
  {                            \
    if (!Serial)               \
    {                          \
      Serial.begin(9600);      \
      Serial.print("\n\n");    \
    }                          \
    Serial.print(__VA_ARGS__); \
  } while (0)
#endif

#define UNIOT_LOG(log_type, log_fmt, log_arg...)                     \
  do                                                                 \
  {                                                                  \
    uniot_log_printf("[" #log_type "][%s:%d][%s] { " log_fmt " }\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##log_arg);   \
  } while (0)

#define UNIOT_LOG_IF(log_type, log_cond, log_fmt, log_arg...) \
  do                                                          \
  {                                                           \
    if (log_cond)                                             \
      UNIOT_LOG(log_type, log_fmt, ##log_arg);                \
  } while (0)

static inline int
uniot_log_printf(const char *format, ...)
{
  va_list arg;
  va_start(arg, format);
  char buf[UNIOT_LOG_BUF_SIZE];
  int len = vsnprintf(buf, sizeof(buf), format, arg);
  va_end(arg);
  UNIOT_LOG_PRINT(buf);
  if (len >= (int)sizeof(buf))
    UNIOT_LOG_PRINT("\n[LOGGER] { buffer too small to show full msg }\n");
  return len;
}

#else
#define UNIOT_LOG_PRINT(...) ((void)0)
#define UNIOT_LOG(...) ((void)0)
#define UNIOT_LOG_IF(...) ((void)0)
#endif

#define UNIOT_LOG_ERROR(...) UNIOT_LOG(ERROR, __VA_ARGS__)
#define UNIOT_LOG_WARN(...) UNIOT_LOG(WARN, __VA_ARGS__)
#define UNIOT_LOG_DEBUG(...) UNIOT_LOG(DEBUG, __VA_ARGS__)
#define UNIOT_LOG_INFO(...) UNIOT_LOG(INFO, __VA_ARGS__)

#define UNIOT_LOG_ERROR_IF(log_cond, log...) UNIOT_LOG_IF(ERROR, log_cond, ##log)
#define UNIOT_LOG_WARN_IF(log_cond, log...) UNIOT_LOG_IF(WARN, log_cond, ##log)
#define UNIOT_LOG_DEBUG_IF(log_cond, log...) UNIOT_LOG_IF(DEBUG, log_cond, ##log)
#define UNIOT_LOG_INFO_IF(log_cond, log...) UNIOT_LOG_IF(INFO, log_cond, ##log)
