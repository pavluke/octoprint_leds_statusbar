/**
 * @file logger.h
 * @brief A simple leveled Serial logger (DEBUG/INFO/WARNING/ERROR).
 *
 * Messages below the configured LOG_LEVEL are completely excluded from the
 * build — the compiler drops them as dead code (the macro expands to
 * `do {} while(0)`, with no call to Serial.print), rather than just being
 * filtered at runtime. This matters on the ESP8266: less code means less
 * flash usage, and no time is spent formatting strings that would never be
 * printed anyway.
 *
 * Usage:
 * @code
 *   LOG_DEBUG("debug details");
 *   LOG_INFO(String("status: ") + someValue);
 *   LOG_WARNING("something went wrong, but not critical");
 *   LOG_ERROR("critical error");
 * @endcode
 *
 * To change the logging level, define LOG_LEVEL before including this file
 * (e.g. at the top of your .ino):
 * @code
 *   #define LOG_LEVEL LOG_LEVEL_DEBUG  // maximum detail
 *   #include "logger.h"
 * @endcode
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

#define LOG_LEVEL_DEBUG   0  ///< Verbose diagnostics for debugging.
#define LOG_LEVEL_INFO    1  ///< Normal events (connection, status changes).
#define LOG_LEVEL_WARNING 2  ///< Recoverable issues (a failed retry attempt).
#define LOG_LEVEL_ERROR   3  ///< Critical errors (connection lost, connect failure).
#define LOG_LEVEL_NONE    4  ///< Disable logging entirely.

#ifndef LOG_LEVEL
/** Default logging level if not explicitly set before #include. */
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

/** Prints a "[LEVEL][millis] " prefix before the message. */
#define LOG_PRINT_PREFIX(tag) \
  do { \
    Serial.print('['); \
    Serial.print(tag); \
    Serial.print("]["); \
    Serial.print(millis()); \
    Serial.print("] "); \
  } while (0)

#if LOG_LEVEL <= LOG_LEVEL_DEBUG
  /** DEBUG-level log. Compiles to a no-op if LOG_LEVEL is above DEBUG. */
  #define LOG_DEBUG(msg) do { LOG_PRINT_PREFIX("DEBUG"); Serial.println(msg); } while (0)
#else
  #define LOG_DEBUG(msg) do {} while (0)
#endif

#if LOG_LEVEL <= LOG_LEVEL_INFO
  /** INFO-level log. Compiles to a no-op if LOG_LEVEL is above INFO. */
  #define LOG_INFO(msg) do { LOG_PRINT_PREFIX("INFO"); Serial.println(msg); } while (0)
#else
  #define LOG_INFO(msg) do {} while (0)
#endif

#if LOG_LEVEL <= LOG_LEVEL_WARNING
  /** WARNING-level log. Compiles to a no-op if LOG_LEVEL is above WARNING. */
  #define LOG_WARNING(msg) do { LOG_PRINT_PREFIX("WARNING"); Serial.println(msg); } while (0)
#else
  #define LOG_WARNING(msg) do {} while (0)
#endif

#if LOG_LEVEL <= LOG_LEVEL_ERROR
  /** ERROR-level log. Compiles to a no-op if LOG_LEVEL is above ERROR. */
  #define LOG_ERROR(msg) do { LOG_PRINT_PREFIX("ERROR"); Serial.println(msg); } while (0)
#else
  #define LOG_ERROR(msg) do {} while (0)
#endif

#endif  // LOGGER_H
