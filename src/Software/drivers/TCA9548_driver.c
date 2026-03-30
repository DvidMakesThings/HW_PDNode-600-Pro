/**
 * @file drivers/TCA9548_driver.c
 * @brief TCA9548ARGER 8-channel I2C mux driver.
 *
 * Writes a single byte to the TCA9548 control register to select which
 * downstream I2C channel is active. Bit N of the byte = enable channel N.
 *
 * @project PDNode-600 Pro
 * @version 1.0.0
 */

#include "../CONFIG.h"
#include "TCA9548_driver.h"
#include "i2c_bus.h"

#define TCA9548_TAG     "[TCA9548]"
#define TCA9548_TIMEOUT 5000 /* µs */

static uint8_t s_active_channel = 0xFF; /* 0xFF = none active */

/* -------------------------------------------------------------------------- */
/*  Private: write control byte directly                                      */
/* -------------------------------------------------------------------------- */
static bool tca_write(uint8_t ctrl_byte) {
    int ret = i2c_bus_write_timeout_us(TCA9548A_I2C_INSTANCE,
                                       TCA9548A_I2C_ADDR,
                                       &ctrl_byte, 1,
                                       false,
                                       TCA9548_TIMEOUT);
    return (ret == 1);
}

/* -------------------------------------------------------------------------- */
/*  Public API                                                                */
/* -------------------------------------------------------------------------- */

bool TCA9548_Init(void) {
    if (!tca_write(0x00)) {
        WARNING_PRINT("%s Not found at 0x%02X\r\n",
                      TCA9548_TAG, TCA9548A_I2C_ADDR);
        return false;
    }
    s_active_channel = 0xFF;
    INFO_PRINT("%s Initialised at 0x%02X\r\n",
               TCA9548_TAG, TCA9548A_I2C_ADDR);
    return true;
}

bool TCA9548_SelectChannel(uint8_t channel) {
    uint8_t ctrl = (channel < 8) ? (uint8_t)(1u << channel) : 0x00;
    if (!tca_write(ctrl)) return false;
    s_active_channel = (channel < 8) ? channel : 0xFF;
    return true;
}

bool TCA9548_DisableAll(void) {
    if (!tca_write(0x00)) return false;
    s_active_channel = 0xFF;
    return true;
}

uint8_t TCA9548_GetActiveChannel(void) { return s_active_channel; }
