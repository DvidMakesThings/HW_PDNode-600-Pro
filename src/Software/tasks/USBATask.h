/**
 * @file tasks/USBATask.h
 * @brief USB-A port monitoring: current sensing via PAC1720, enable/fault via MCP23017.
 *
 * Hardware:
 *   - MCP23017 @ 0x27 (I2C1): Port A pins 0-3 = USB-A enable, Port B pins 0-3 = fault
 *   - PAC1720 @ 0x4C: channels 1/2 = USB-A port 0/1
 *   - PAC1720 @ 0x4D: channels 1/2 = USB-A port 2/3
 *
 * @project PDNode-600 Pro
 */

#pragma once
#include "../CONFIG.h"

#ifdef __cplusplus
extern "C" {
#endif

#define USBA_NUM_PORTS 4

/** Live telemetry for a single USB-A port. */
typedef struct {
    bool    enabled;      /**< Enable pin state (MCP23017 GPA) */
    bool    fault;        /**< Fault pin state (MCP23017 GPB, active low) */
    float   current_a;   /**< Current measured by PAC1720 */
    float   voltage_v;   /**< Bus voltage (fixed 5.0 V for USB-A) */
    float   power_w;     /**< Calculated power */
    uint32_t uptime_s;   /**< Seconds port has been enabled */
} usba_telemetry_t;

/** Create and start USB-A monitoring task. */
BaseType_t USBATask_Init(bool enable);

/** Returns true once task is ready. */
bool USBA_IsReady(void);

/** Get telemetry for a port (0-3). */
bool USBA_GetTelemetry(uint8_t port, usba_telemetry_t *out);

/** Enable or disable a USB-A port via MCP23017 GPIO. */
bool USBA_SetEnable(uint8_t port, bool on);

#ifdef __cplusplus
}
#endif
