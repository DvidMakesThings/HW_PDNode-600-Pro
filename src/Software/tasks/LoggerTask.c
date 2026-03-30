/**
 * @file tasks/LoggerTask.c
 * @brief Queue-based USB-CDC / UART logger for PDNode-600 Pro.
 *
 * All log_printf() calls are safe from any task. Messages are queued and
 * printed from this dedicated task, avoiding USB-CDC stalls in callers.
 *
 * @project PDNode-600 Pro
 * @version 1.0.0
 */

#include "LoggerTask.h"
#include "../CONFIG.h"
#include "tusb.h"

#define LOGGER_TAG "[LOGGER]"

typedef struct {
    char msg[LOGGER_MSG_MAX];
} LogItem_t;

static QueueHandle_t s_log_queue = NULL;
static TaskHandle_t s_log_task = NULL;
static volatile uint32_t s_mute_depth = 0u;

/* -------------------------------------------------------------------------- */

static void LoggerTask_Function(void *arg) {
    (void)arg;
    LogItem_t item;
    for (;;) {
        if (!tud_cdc_connected()) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        if (xQueueReceive(s_log_queue, &item, pdMS_TO_TICKS(100)) == pdPASS) {
            uint32_t len = (uint32_t)strlen(item.msg);
            uint32_t sent = 0u;
            while (sent < len) {
                sent += tud_cdc_write(item.msg + sent, len - sent);
                tud_cdc_write_flush();
                if (sent < len) {
                    vTaskDelay(pdMS_TO_TICKS(1));
                }
            }
        }
    }
}

/* -------------------------------------------------------------------------- */
/*  Public API                                                                */
/* -------------------------------------------------------------------------- */

bool Logger_IsReady(void) { return (s_log_queue != NULL); }

BaseType_t LoggerTask_Init(bool enable) {
    if (!enable)
        return pdPASS;

    if (s_log_queue == NULL) {
        s_log_queue = xQueueCreate(LOGGER_QUEUE_LEN, sizeof(LogItem_t));
        if (s_log_queue == NULL)
            return pdFAIL;
    }

    if (s_log_task == NULL) {
        if (xTaskCreate(LoggerTask_Function, "Logger", LOGGER_STACK_SIZE, NULL, LOGTASK_PRIORITY,
                        &s_log_task) != pdPASS) {
            return pdFAIL;
        }
    }
    return pdPASS;
}

void log_printf(const char *fmt, ...) {
    if (s_log_queue == NULL || fmt == NULL)
        return;
    if (s_mute_depth != 0u)
        return;

    LogItem_t item;
    va_list args;
    va_start(args, fmt);
    vsnprintf(item.msg, sizeof(item.msg), fmt, args);
    va_end(args);
    xQueueSend(s_log_queue, &item, 0);
}

void log_printf_force(const char *fmt, ...) {
    if (s_log_queue == NULL || fmt == NULL)
        return;

    LogItem_t item;
    va_list args;
    va_start(args, fmt);
    vsnprintf(item.msg, sizeof(item.msg), fmt, args);
    va_end(args);
    xQueueSend(s_log_queue, &item, 0);
}

void Logger_MutePush(void) {
    taskENTER_CRITICAL();
    s_mute_depth++;
    taskEXIT_CRITICAL();
}

void Logger_MutePop(void) {
    taskENTER_CRITICAL();
    if (s_mute_depth > 0u)
        s_mute_depth--;
    taskEXIT_CRITICAL();
}
