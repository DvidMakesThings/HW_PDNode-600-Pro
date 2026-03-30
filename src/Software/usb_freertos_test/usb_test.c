/**
 * @file usb_test.c
 * @brief Minimal USB CDC enumeration test for RP2354B (BladeCore-M54E).
 *
 * No FreeRTOS. No drivers. Just bare-metal USB CDC on pico2.
 * If you see "PDNode USB Test" in a terminal, USB is working.
 */

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include <stdio.h>

int main(void) {
    set_sys_clock_khz(200000, true);
    stdio_init_all();

    /* Wait up to 5 s for the host to open the port, then fall through */
    for (int i = 0; i < 100; i++) {
        if (stdio_usb_connected()) break;
        sleep_ms(50);
    }

    uint32_t count = 0;
    while (true) {
        printf("[PDNode USB Test] count=%lu\r\n", (unsigned long)count++);
        sleep_ms(1000);
    }
    return 0;
}
