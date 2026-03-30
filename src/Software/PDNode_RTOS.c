/**
 * @file PDNode_RTOS.c
 * @brief PDNode-600 Pro — FreeRTOS firmware entry point.
 *
 * Sets up the system clock, creates the InitTask (which bootstraps all other
 * tasks in sequence), and starts the FreeRTOS scheduler.
 *
 * @project PDNode-600 Pro
 * @version 1.0.0
 */

#include "CONFIG.h"
#include "tasks/InitTask.h"
#include "tasks/LoggerTask.h"
#include "tusb.h"

/* --------------------------------------------------------------------------
 *  Persistent boot-trigger word (survives watchdog reset, placed in
 *  .uninitialized_data so the linker does not zero it on startup).
 * -------------------------------------------------------------------------- */
__attribute__((section(".uninitialized_data"))) volatile uint32_t bootloader_trigger;

/* --------------------------------------------------------------------------
 *  USB device task — runs TinyUSB stack under FreeRTOS
 * -------------------------------------------------------------------------- */
static void usb_device_task(void *param) {
    (void)param;
    tusb_rhport_init_t dev_init = {.role = TUSB_ROLE_DEVICE, .speed = TUSB_SPEED_AUTO};
    tusb_init(0, &dev_init);
    for (;;) {
        tud_task();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/* --------------------------------------------------------------------------
 *  FreeRTOS hook implementations
 * -------------------------------------------------------------------------- */

void vAssertCalled(const char *file, int line) {
    (void)file;
    (void)line;
    taskDISABLE_INTERRUPTS();
    /* Spin — a connected debugger can inspect the call stack */
    for (;;) {
        tight_loop_contents();
    }
}

void vApplicationIdleHook(void) { /* Nothing — watchdog is fed by HealthTask, not the idle hook */ }

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    /* Log then reboot */
    log_printf_force("[FATAL] Stack overflow in task: %s\r\n", pcTaskName);
    vTaskDelay(pdMS_TO_TICKS(50));
    watchdog_reboot(0, 0, 100);
    for (;;) {
        tight_loop_contents();
    }
}

void vApplicationMallocFailedHook(void) {
    log_printf_force("[FATAL] FreeRTOS malloc failed\r\n");
    vTaskDelay(pdMS_TO_TICKS(50));
    watchdog_reboot(0, 0, 100);
    for (;;) {
        tight_loop_contents();
    }
}

/* --------------------------------------------------------------------------
 *  main
 * -------------------------------------------------------------------------- */

int main(void) {
    /* Run RP2354B at 200 MHz */
    set_sys_clock_khz(200000, true);

    xTaskCreate(usb_device_task, "usb", 2048, NULL, configMAX_PRIORITIES - 1, NULL);
    InitTask_Create();

    /* Start scheduler — does not return */
    vTaskStartScheduler();

    /* Should never reach here */
    for (;;) {
        tight_loop_contents();
    }
    return 0;
}
