/**
 * @file tasks/HeartbeatTask.c
 * @brief PWM breathing LED on PIN_HEARTBEAT (GPIO36).
 *
 * Produces a smooth 0→100%→0 sine-ish ramp at ~1 Hz using PWM hardware.
 * PWM slice 2 channel A on RP2354B (gpio >> 1 = slice, gpio & 1 = channel).
 *
 * @project PDNode-600 Pro
 * @version 1.0.0
 */

#include "../CONFIG.h"
#include "HeartbeatTask.h"
#include "HealthTask.h"

#define HB_TAG          "[HB]"
#define HB_STACK        512
#define HB_STEPS        100      /* steps per half-cycle */
#define HB_STEP_MS      HEARTBEAT_FADE_STEP_MS
#define HB_PWM_WRAP     1000u

static volatile bool s_ready = false;

static void HeartbeatTask_Function(void *arg) {
    (void)arg;

    /* Configure PWM on PIN_HEARTBEAT */
    gpio_set_function(PIN_HEARTBEAT, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(PIN_HEARTBEAT);
    uint chan  = pwm_gpio_to_channel(PIN_HEARTBEAT);

    pwm_config cfg = pwm_get_default_config();
    /* clkdiv = 125e6 / (1000 * 1000) = 125 → ~1 kHz PWM */
    pwm_config_set_clkdiv(&cfg, 125.0f);
    pwm_config_set_wrap(&cfg, HB_PWM_WRAP - 1);
    pwm_init(slice, &cfg, true);
    pwm_set_chan_level(slice, chan, 0);

    INFO_PRINT("%s Heartbeat LED started on GPIO%d (slice %u)\r\n",
               HB_TAG, PIN_HEARTBEAT, slice);

    s_ready = true;
    uint32_t hb_ms = 0;

    for (;;) {
        uint32_t now_ms = to_ms_since_boot(get_absolute_time());
        if ((now_ms - hb_ms) >= 1000u) {
            hb_ms = now_ms;
            Health_Heartbeat(HEALTH_ID_HEARTBEAT);
        }

        /* Fade up */
        for (int i = 0; i <= HB_STEPS; i++) {
            uint32_t level = (uint32_t)(((uint64_t)i * i * (HB_PWM_WRAP - 1))
                                        / (HB_STEPS * HB_STEPS));
            pwm_set_chan_level(slice, chan, (uint16_t)level);
            vTaskDelay(pdMS_TO_TICKS(HB_STEP_MS));
        }

        /* Fade down */
        for (int i = HB_STEPS; i >= 0; i--) {
            uint32_t level = (uint32_t)(((uint64_t)i * i * (HB_PWM_WRAP - 1))
                                        / (HB_STEPS * HB_STEPS));
            pwm_set_chan_level(slice, chan, (uint16_t)level);
            vTaskDelay(pdMS_TO_TICKS(HB_STEP_MS));
        }

        vTaskDelay(pdMS_TO_TICKS(200)); /* brief off-pause between beats */
    }
}

BaseType_t HeartbeatTask_Init(bool enable) {
    if (!enable) return pdPASS;
    return xTaskCreate(HeartbeatTask_Function, "Heartbeat", HB_STACK,
                       NULL, HEARTBEATTASK_PRIORITY, NULL);
}

bool Heartbeat_IsReady(void) { return s_ready; }
