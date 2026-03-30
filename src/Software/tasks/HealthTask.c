/**
 * @file tasks/HealthTask.c
 * @brief Hardware watchdog manager and per-task liveness monitor.
 *
 * The watchdog is only armed after HEALTH_WARMUP_MS has elapsed and each
 * registered task has sent at least one heartbeat. While armed, each task
 * must heartbeat within HEALTH_SILENCE_MS or the watchdog fires.
 *
 * @project PDNode-600 Pro
 * @version 1.0.0
 */

#include "HealthTask.h"
#include "../CONFIG.h"

#define HEALTH_TAG "[HEALTH]"
#define HEALTH_STACK 1024
#define HEALTH_DIAG_MS 30000 /* print diagnostics every 30 s */

/* -------------------------------------------------------------------------- */
/*  Per-task record                                                           */
/* -------------------------------------------------------------------------- */
typedef struct {
    TaskHandle_t handle;
    const char *name;
    uint32_t last_beat_ms;
    bool registered;
    bool ever_beat;
} health_record_t;

static health_record_t s_tasks[HEALTH_ID_MAX];
static TaskHandle_t s_health_task = NULL;

/* -------------------------------------------------------------------------- */
/*  Blocked event ring                                                        */
/* -------------------------------------------------------------------------- */
#define BLOCKED_RING 8
typedef struct {
    char tag[20];
    uint32_t ms;
} blocked_evt_t;
static blocked_evt_t s_blocked[BLOCKED_RING];
static uint8_t s_blocked_head = 0;

/* -------------------------------------------------------------------------- */

static bool all_ever_beat(void) {
    for (int i = 0; i < HEALTH_ID_MAX; i++) {
        if (s_tasks[i].registered && !s_tasks[i].ever_beat)
            return false;
    }
    return true;
}

static bool all_alive(uint32_t now_ms) {
    for (int i = 0; i < HEALTH_ID_MAX; i++) {
        if (!s_tasks[i].registered)
            continue;
        if ((now_ms - s_tasks[i].last_beat_ms) > HEALTH_SILENCE_MS)
            return false;
    }
    return true;
}

static void print_diagnostics(void) {
    HEALTH_INFO("%s === Diagnostics ===\r\n", HEALTH_TAG);
    HEALTH_INFO("%s Free heap: %u bytes\r\n", HEALTH_TAG, (unsigned)xPortGetFreeHeapSize());
    for (int i = 0; i < HEALTH_ID_MAX; i++) {
        if (!s_tasks[i].registered)
            continue;
        UBaseType_t hwm = uxTaskGetStackHighWaterMark(s_tasks[i].handle);
        HEALTH_INFO("%s   %-16s stack HWM: %u words\r\n", HEALTH_TAG, s_tasks[i].name,
                    (unsigned)hwm);
    }
}

static void HealthTask_Function(void *arg) {
    (void)arg;

    uint32_t boot_ms = to_ms_since_boot(get_absolute_time());
    bool wdt_armed = false;
    uint32_t last_diag_ms = boot_ms;

    HEALTH_INFO("%s Task started (warmup %u ms)\r\n", HEALTH_TAG, HEALTH_WARMUP_MS);

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(500));
        uint32_t now_ms = to_ms_since_boot(get_absolute_time());

        /* Arm watchdog after warmup + all tasks have beaten at least once */
        if (!wdt_armed) {
            bool warmed = (now_ms - boot_ms) >= HEALTH_WARMUP_MS;
            bool all_hb = all_ever_beat();
            if (warmed && all_hb) {
                watchdog_enable(HEALTH_SILENCE_MS + 2000, true);
                wdt_armed = true;
                HEALTH_INFO("%s Watchdog armed (%u ms timeout)\r\n", HEALTH_TAG,
                            HEALTH_SILENCE_MS + 2000);
            }
        }

        /* Feed watchdog only if all tasks are alive */
        if (wdt_armed) {
            if (all_alive(now_ms)) {
                watchdog_update();
            } else {
                /* Don't feed — watchdog fires and resets the system */
                WARNING_PRINT("%s Liveness failure! Watchdog will fire.\r\n", HEALTH_TAG);
                for (int i = 0; i < HEALTH_ID_MAX; i++) {
                    if (!s_tasks[i].registered)
                        continue;
                    uint32_t delta = now_ms - s_tasks[i].last_beat_ms;
                    if (delta > HEALTH_SILENCE_MS) {
                        WARNING_PRINT("%s   SILENT: %s (%u ms)\r\n", HEALTH_TAG, s_tasks[i].name,
                                      (unsigned)delta);
                    }
                }
            }
        }

        /* Periodic diagnostics */
        if ((now_ms - last_diag_ms) >= HEALTH_DIAG_MS) {
            last_diag_ms = now_ms;
            print_diagnostics();
        }
    }
}

/* -------------------------------------------------------------------------- */
/*  Public API                                                                */
/* -------------------------------------------------------------------------- */

void HealthTask_Start(void) {
    if (s_health_task != NULL)
        return; /* idempotent */
    xTaskCreate(HealthTask_Function, "Health", HEALTH_STACK, NULL, HEALTHTASK_PRIORITY,
                &s_health_task);
}

void Health_RegisterTask(health_id_t id, TaskHandle_t h, const char *name) {
    if ((unsigned)id >= HEALTH_ID_MAX)
        return;
    s_tasks[id].handle = h;
    s_tasks[id].name = name;
    s_tasks[id].last_beat_ms = to_ms_since_boot(get_absolute_time());
    s_tasks[id].registered = true;
    s_tasks[id].ever_beat = false;
}

void Health_Heartbeat(health_id_t id) {
    if ((unsigned)id >= HEALTH_ID_MAX)
        return;
    s_tasks[id].last_beat_ms = to_ms_since_boot(get_absolute_time());
    s_tasks[id].ever_beat = true;
}

void Health_RecordBlocked(const char *tag, uint32_t waited_ms) {
    blocked_evt_t *e = &s_blocked[s_blocked_head % BLOCKED_RING];
    strncpy(e->tag, tag ? tag : "?", sizeof(e->tag) - 1);
    e->tag[sizeof(e->tag) - 1] = '\0';
    e->ms = waited_ms;
    s_blocked_head++;
    WARNING_PRINT("%s Blocked: %s for %u ms\r\n", HEALTH_TAG, e->tag, (unsigned)waited_ms);
}

void Health_RebootNow(const char *reason) {
    HEALTH_INFO("%s Rebooting: %s\r\n", HEALTH_TAG, reason ? reason : "(no reason)");
    vTaskDelay(pdMS_TO_TICKS(100));
    watchdog_enable(1, true);
    for (;;)
        tight_loop_contents();
}
