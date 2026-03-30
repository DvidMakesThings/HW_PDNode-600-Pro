/**
 * @file drivers/PAC1720_driver.h
 * @brief PAC1720 dual-channel current/power monitor driver (I2C).
 *
 * Two PAC1720 devices are used for USB-A current monitoring:
 *   - PAC1720 #1 @ 0x4C: USB-A port 0 (ch1) and port 1 (ch2)
 *   - PAC1720 #2 @ 0x4D: USB-A port 2 (ch1) and port 3 (ch2)
 *
 * Sense resistor: 0.02 Ω (set via PAC1720_RSENSE_OHM in CONFIG.h)
 * Full-scale current range: ±80 mV / 0.02 Ω = ±4 A
 *
 * @project PDNode-600 Pro
 */

#pragma once
#include "../CONFIG.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PAC1720 Register Map (DS20005386C Table 6-1) */
#define PAC1720_REG_PROD_ID         0xFD
#define PAC1720_REG_CONFIG          0x00
#define PAC1720_REG_CONV_RATE       0x01
#define PAC1720_REG_VSRC_SAMP_CFG  0x0A   /* VSOURCE sampling — both channels combined */
#define PAC1720_REG_CH1_SENSE_CFG   0x0B   /* CH1 VSENSE sampling/range config */
#define PAC1720_REG_CH2_SENSE_CFG   0x0C   /* CH2 VSENSE sampling/range config */
#define PAC1720_REG_CH1_VSENSE_HI   0x0D   /* CH1 sense result high byte */
#define PAC1720_REG_CH1_VSENSE_LO   0x0E   /* CH1 sense result low byte  */
#define PAC1720_REG_CH2_VSENSE_HI   0x0F   /* CH2 sense result high byte */
#define PAC1720_REG_CH2_VSENSE_LO   0x10   /* CH2 sense result low byte  */
#define PAC1720_REG_CH1_VSRC_HI     0x11   /* CH1 VSOURCE result high byte */
#define PAC1720_REG_CH1_VSRC_LO     0x12   /* CH1 VSOURCE result low byte  */
#define PAC1720_REG_CH2_VSRC_HI     0x13   /* CH2 VSOURCE result high byte */
#define PAC1720_REG_CH2_VSRC_LO     0x14   /* CH2 VSOURCE result low byte  */

/* Full-scale sense voltage (mV) for the chosen range */
#define PAC1720_FSR_MV              80.0f   /* ±80 mV range */

/**
 * @brief Initialise a PAC1720 device.
 * @param i2c_addr  7-bit I2C address (PAC1720_1_I2C_ADDR or PAC1720_2_I2C_ADDR)
 * @return true on success
 */
bool PAC1720_Init(uint8_t i2c_addr);

/**
 * @brief Read current from one channel.
 * @param i2c_addr  Device I2C address
 * @param channel   1 or 2
 * @param current_a Output: current in Amps (positive = flowing into load)
 * @return true on success
 */
bool PAC1720_ReadCurrent(uint8_t i2c_addr, uint8_t channel, float *current_a);

/**
 * @brief Read source voltage from one channel.
 * @param i2c_addr  Device I2C address
 * @param channel   1 or 2
 * @param voltage_v Output: source voltage in Volts
 * @return true on success
 */
bool PAC1720_ReadVoltage(uint8_t i2c_addr, uint8_t channel, float *voltage_v);

#ifdef __cplusplus
}
#endif
