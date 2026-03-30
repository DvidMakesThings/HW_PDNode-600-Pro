/**
 * @file drivers/TCA9548_driver.h
 * @brief TCA9548ARGER 8-channel I2C multiplexer driver.
 *
 * The TCA9548A is used to expose individual I2C buses to each of the
 * 8 PDCard slots. Only one channel is active at a time.
 *
 * Address: 0x70 (A0=A1=A2=GND per CONFIG.h TCA9548A_I2C_ADDR)
 * Bus:     I2C1 (TCA9548A_I2C_INSTANCE)
 *
 * @project PDNode-600 Pro
 */

#pragma once
#include "../CONFIG.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Initialise TCA9548A (all channels disabled). */
bool TCA9548_Init(void);

/**
 * @brief Select a single I2C channel (0-7).
 * @param channel  Channel to enable (0-7), or 8 to disable all.
 * @return true on success.
 */
bool TCA9548_SelectChannel(uint8_t channel);

/** Disable all channels. */
bool TCA9548_DisableAll(void);

/** Return the currently active channel (0-7), or 0xFF if none. */
uint8_t TCA9548_GetActiveChannel(void);

#ifdef __cplusplus
}
#endif
