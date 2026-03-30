/**
 * @file tasks/LoggerTask.h
 * @brief Queue-based UART/USB-CDC logger task.
 *
 * log_printf() is safe to call from any task context — messages are queued
 * and emitted from the dedicated logger task.
 *
 * @project PDNode-600 Pro
 */

#pragma once
#include "../CONFIG.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Create and start the logger task. Call before any other task. */
BaseType_t LoggerTask_Init(bool enable);

/** Returns true once the log queue has been created (early boot ready check). */
bool Logger_IsReady(void);

/** Queue a formatted log message (non-blocking, drops if queue full). */
void log_printf(const char *fmt, ...);

/** Queue a formatted log message ignoring the mute flag. */
void log_printf_force(const char *fmt, ...);

/** Increment mute depth — log output suppressed while depth > 0. */
void Logger_MutePush(void);

/** Decrement mute depth. */
void Logger_MutePop(void);

#ifdef __cplusplus
}
#endif
