#include "FreeRTOS.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include "task.h"
#include "tusb.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void usb_task(void *param);
static void cdc_task(void *param);

/* -------------------------------------------------------------------------- */
/*  FreeRTOS hooks                                                             */
/* -------------------------------------------------------------------------- */
void vApplicationMallocFailedHook(void) {
    taskDISABLE_INTERRUPTS();
    for (;;) {}
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name) {
    (void)task;
    (void)name;
    taskDISABLE_INTERRUPTS();
    for (;;) {}
}

void vAssertCalled(const char *file, int line) {
    (void)file;
    (void)line;
    taskDISABLE_INTERRUPTS();
    for (;;) {}
}

/* -------------------------------------------------------------------------- */
/*  USB task                                                                   */
/* -------------------------------------------------------------------------- */
static void usb_task(void *param) {
    (void)param;

    tusb_rhport_init_t dev_init = {.role = TUSB_ROLE_DEVICE, .speed = TUSB_SPEED_AUTO};

    tusb_init(0, &dev_init);

    for (;;) {
        tud_task();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/* -------------------------------------------------------------------------- */
/*  CDC task                                                                   */
/* -------------------------------------------------------------------------- */
static void cdc_task(void *param) {
    (void)param;

    uint32_t counter = 0u;
    bool banner_sent = false;

    for (;;) {
        if (tud_cdc_connected()) {
            if (!banner_sent) {
                const char *banner = "PDNode FreeRTOS CDC online\r\n";
                tud_cdc_write(banner, (uint32_t)strlen(banner));
                tud_cdc_write_flush();
                banner_sent = true;
            }

            while (tud_cdc_available()) {
                uint8_t buf[64];
                uint32_t count = tud_cdc_read(buf, sizeof(buf));
                if (count > 0u) {
                    tud_cdc_write(buf, count);
                    tud_cdc_write_flush();
                }
            }

            {
                char msg[64];
                int len = snprintf(msg, sizeof(msg), "tick %lu\r\n", (unsigned long)counter++);
                if (len > 0) {
                    tud_cdc_write(msg, (uint32_t)len);
                    tud_cdc_write_flush();
                }
            }
        } else {
            banner_sent = false;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* -------------------------------------------------------------------------- */
/*  Main                                                                       */
/* -------------------------------------------------------------------------- */
int main(void) {
    set_sys_clock_khz(200000, true);

    irq_set_priority(USBCTRL_IRQ, 0x00);

    xTaskCreate(usb_task, "usb", 2048, NULL, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(cdc_task, "cdc", 1024, NULL, configMAX_PRIORITIES - 2, NULL);

    vTaskStartScheduler();

    for (;;) {}
}