/**
 * @file tasks/PDCardTask.h
 * @brief USB-C PD Card monitoring task (8 ports via SPI + TCA9548 I2C mux).
 *
 * Each PDCard slot has:
 *   - SPI chip select (SPI_CS_PORT0..7)
 *   - I2C channel via TCA9548 (ports 0-7)
 *   - PMIC voltage monitor via CD74HC4067 analog mux
 *   - VBUS voltage monitor via CD74HC4067
 *
 * This task is currently a PLACEHOLDER — PDCard SPI protocol and register
 * map will be implemented in a future firmware revision once the PDCard
 * hardware is finalised.
 *
 * @project PDNode-600 Pro
 */

#pragma once
#include "../CONFIG.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PDCARD_NUM_PORTS 8

/** Live telemetry for a single PDCard port. */
typedef struct {
    bool    connected;       /**< Device detected on port */
    bool    pd_active;       /**< PD contract negotiated */
    float   vbus_v;          /**< VBUS voltage measured by ADC mux */
    float   pmic_v;          /**< PMIC output voltage from ADC mux */
    float   current_a;       /**< Port current (placeholder) */
    float   power_w;         /**< Port power = vbus_v * current_a */
    uint32_t uptime_s;       /**< Seconds since last connection */
    char    port_state[16];  /**< "Disconnected", "Negotiating", "Ready", "Fault" */
    char    contract[32];    /**< PD contract string, e.g. "20V 3A (60W)" */
} pdcard_telemetry_t;

/** Create and start the PDCard monitoring task. */
BaseType_t PDCardTask_Init(bool enable);

/** Returns true once the task has been created. */
bool PDCard_IsReady(void);

/**
 * @brief Get telemetry snapshot for a single port.
 * @param port  Port index 0-7
 * @param out   Pointer to receive telemetry
 * @return true on success
 */
bool PDCard_GetTelemetry(uint8_t port, pdcard_telemetry_t *out);

#ifdef __cplusplus
}
#endif
