/**
 * @file tasks/HealthTask.h
 * @brief System health monitor: watchdog feed, per-task heartbeats, diagnostics.
 *
 * Usage:
 *  1) Health_RegisterTask(id, handle, "name") after xTaskCreate for each task.
 *  2) HealthTask_Start() once all tasks are created (called from InitTask).
 *  3) Inside each task loop: Health_Heartbeat(HEALTH_ID_xxx) once per iteration.
 *
 * @project PDNode-600 Pro
 */

#pragma once
#include "../CONFIG.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/*  Heartbeat cadence expectations (ms)                                       */
/* -------------------------------------------------------------------------- */
#define HEALTH_LOGTASK_MS       1000
#define HEALTH_CONSOLETASK_MS   1000
#define HEALTH_NETTASK_MS       500
#define HEALTH_STORAGETASK_MS   1000
#define HEALTH_PDCARDTASK_MS    2000
#define HEALTH_USBATASK_MS      2000
#define HEALTH_HEARTBEATTASK_MS 2000

/* Internal heartbeat rates used by each task */
#define LOGTASKBEAT_MS          500
#define CONSOLETASKBEAT_MS      500

/* Watchdog timeout — all tasks must heartbeat within this window */
#define HEALTH_SILENCE_MS       8000
/* Warm-up period before watchdog is armed */
#define HEALTH_WARMUP_MS        15000

/* -------------------------------------------------------------------------- */
/*  Task health IDs                                                           */
/* -------------------------------------------------------------------------- */
typedef enum {
    HEALTH_ID_LOGGER    = 0,
    HEALTH_ID_CONSOLE   = 1,
    HEALTH_ID_STORAGE   = 2,
    HEALTH_ID_NET       = 3,
    HEALTH_ID_PDCARD    = 4,
    HEALTH_ID_USBA      = 5,
    HEALTH_ID_HEARTBEAT = 6,
    HEALTH_ID_MAX       = 16
} health_id_t;

/* -------------------------------------------------------------------------- */
/*  Public API                                                                */
/* -------------------------------------------------------------------------- */

/** Create and start the health monitoring task. */
void HealthTask_Start(void);

/** Register a task for liveness tracking. */
void Health_RegisterTask(health_id_t id, TaskHandle_t h, const char *name);

/** Signal that a task is still alive. Call from every task's main loop. */
void Health_Heartbeat(health_id_t id);

/** Record a long blocking event for diagnostics. */
void Health_RecordBlocked(const char *tag, uint32_t waited_ms);

/** Reboot the system immediately with a reason string. */
void Health_RebootNow(const char *reason);

#ifdef __cplusplus
}
#endif
