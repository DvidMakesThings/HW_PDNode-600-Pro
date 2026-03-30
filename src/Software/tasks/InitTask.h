/**
 * @file tasks/InitTask.h
 * @brief System bring-up sequencer task.
 *
 * InitTask runs at the highest priority and:
 *  1) Initialises all hardware peripherals (GPIO, I2C, SPI, ADC).
 *  2) Probes peripherals (EEPROM, MCP23017, W5500).
 *  3) Creates and waits for each subsystem task in dependency order.
 *  4) Registers all tasks with HealthTask, then starts HealthTask.
 *  5) Deletes itself once the system is fully operational.
 *
 * @project PDNode-600 Pro
 */

#pragma once
#include "../CONFIG.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Create InitTask. Call from main() before vTaskStartScheduler(). */
void InitTask_Create(void);

#ifdef __cplusplus
}
#endif
