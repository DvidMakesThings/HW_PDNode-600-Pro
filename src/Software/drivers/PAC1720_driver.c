/**
 * @file drivers/PAC1720_driver.c
 * @brief PAC1720 dual-channel current/power monitor driver.
 *
 * Uses i2c_bus wrappers for thread-safe access on I2C1.
 *
 * @project PDNode-600 Pro
 * @version 1.0.0
 */

#include "../CONFIG.h"
#include "PAC1720_driver.h"
#include "i2c_bus.h"

#define PAC1720_TAG     "[PAC1720]"
#define PAC1720_TIMEOUT 5000  /* µs */

/* -------------------------------------------------------------------------- */
/*  Private helpers                                                           */
/* -------------------------------------------------------------------------- */

static bool pac_write_reg(uint8_t addr, uint8_t reg, uint8_t val) {
    return i2c_bus_write_reg8(PAC1720_I2C_INSTANCE, addr, reg, val,
                              PAC1720_TIMEOUT);
}

static bool pac_read_reg(uint8_t addr, uint8_t reg, uint8_t *out) {
    return i2c_bus_read_reg8(PAC1720_I2C_INSTANCE, addr, reg, out,
                             PAC1720_TIMEOUT);
}

static bool pac_read16(uint8_t addr, uint8_t reg_hi, int16_t *out) {
    /* Read both bytes in a single atomic I2C transaction (write reg addr,
     * repeated-start, read 2 bytes). Two separate reads risk getting hi and lo
     * bytes from different conversion cycles if an update lands between them. */
    uint8_t buf[2] = {0, 0};
    int ret = i2c_bus_write_read(PAC1720_I2C_INSTANCE, addr,
                                 &reg_hi, 1u, buf, 2u, PAC1720_TIMEOUT);
    if (ret != 2) return false;
    /* 11-bit sign-magnitude result lives in bits [15:5] of the 16-bit word.
     * Arithmetic right-shift to bits [10:0]; sign bit propagates correctly for
     * positive readings (negative currents are clamped to 0 by the caller). */
    int16_t raw = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
    raw >>= 5;
    *out = raw;
    return true;
}

/* -------------------------------------------------------------------------- */
/*  Public API                                                                */
/* -------------------------------------------------------------------------- */

bool PAC1720_Init(uint8_t i2c_addr) {
    /* Verify product ID (should be 0x59) */
    uint8_t pid = 0;
    if (!pac_read_reg(i2c_addr, PAC1720_REG_PROD_ID, &pid)) {
        WARNING_PRINT("%s Device 0x%02X not found\r\n", PAC1720_TAG, i2c_addr);
        return false;
    }
    INFO_PRINT("%s 0x%02X found, PID=0x%02X\r\n", PAC1720_TAG, i2c_addr, pid);

    /* Continuous conversion, 64 samples/s */
    pac_write_reg(i2c_addr, PAC1720_REG_CONV_RATE, 0x03); /* 64 conv/s */

    /* CH1 and CH2 VSENSE: 40 ms sample time, no averaging, ±80 mV FSR
     * Reg 0x0B/0x0C: bits[6:5]=10 (40ms), bits[4:2]=000 (1 sample), bits[1:0]=11 (±80mV) */
    pac_write_reg(i2c_addr, PAC1720_REG_CH1_SENSE_CFG, 0x43);
    pac_write_reg(i2c_addr, PAC1720_REG_CH2_SENSE_CFG, 0x43);

    /* VSOURCE sampling: both channels 10 ms, no averaging
     * Reg 0x0A: bits[7:6]=10 (CH2 10ms), bits[5:3]=000 (1 sample), bits[2:1]=10 (CH1 10ms), bit[0]=0 */
    pac_write_reg(i2c_addr, PAC1720_REG_VSRC_SAMP_CFG, 0x88);

    return true;
}

bool PAC1720_ReadCurrent(uint8_t i2c_addr, uint8_t channel, float *current_a) {
    if (!current_a) return false;
    *current_a = 0.0f;

    uint8_t reg_hi = (channel == 1) ? PAC1720_REG_CH1_VSENSE_HI
                                    : PAC1720_REG_CH2_VSENSE_HI;
    int16_t raw = 0;
    if (!pac_read16(i2c_addr, reg_hi, &raw)) return false;

    /* Vsense (mV) = raw * FSR_mV / 1024  (11-bit, so 2^10 = 1024 codes) */
    float vsense_mv = (float)raw * PAC1720_FSR_MV / 1024.0f;
    *current_a = vsense_mv / 1000.0f / PAC1720_RSENSE_OHM;
    if (*current_a < 0.0f) *current_a = 0.0f; /* clamp negative */
    return true;
}

bool PAC1720_ReadVoltage(uint8_t i2c_addr, uint8_t channel, float *voltage_v) {
    if (!voltage_v) return false;
    *voltage_v = 0.0f;

    uint8_t reg_hi = (channel == 1) ? PAC1720_REG_CH1_VSRC_HI
                                    : PAC1720_REG_CH2_VSRC_HI;
    uint8_t buf[2] = {0, 0};
    int ret = i2c_bus_write_read(PAC1720_I2C_INSTANCE, i2c_addr,
                                 &reg_hi, 1u, buf, 2u, PAC1720_TIMEOUT);
    if (ret != 2) return false;

    /* 10-bit reading in bits[15:6], full scale = 40 V */
    uint16_t raw = (((uint16_t)buf[0] << 2) | (buf[1] >> 6)) & 0x3FF;
    *voltage_v   = (float)raw * 40.0f / 1023.0f;
    return true;
}
