/**
 * @file eeprom/eeprom_io.c
 * @brief Unified EEPROM block I/O with CRC-8 integrity.
 *
 * @project PDNode-600 Pro
 * @version 1.0.0
 */

#include "../CONFIG.h"
#include "eeprom_io.h"
#include "../drivers/CAT24C256_driver.h"

#define EEPROM_TAG "[EEPROM]"

/* -------------------------------------------------------------------------- */
/*  Initialisation                                                             */
/* -------------------------------------------------------------------------- */

void eeprom_init(void) {
    CAT24C256_Init();
}

/* -------------------------------------------------------------------------- */
/*  CRC-8 (polynomial 0x07, SMBUS)                                            */
/* -------------------------------------------------------------------------- */

uint8_t eeprom_crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0x00u;
    for (size_t i = 0u; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0u; j < 8u; j++) {
            crc = (crc & 0x80u) ? (uint8_t)((crc << 1) ^ 0x07u) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

/* -------------------------------------------------------------------------- */
/*  Block I/O                                                                  */
/* -------------------------------------------------------------------------- */

void eeprom_write_block(uint16_t addr, const void *data, uint16_t len) {
    CAT24C256_WriteBuffer(addr, (const uint8_t *)data, len);
    uint8_t crc = eeprom_crc8((const uint8_t *)data, len);
    CAT24C256_WriteBuffer((uint16_t)(addr + len), &crc, 1u);
}

bool eeprom_read_block(uint16_t addr, void *data, uint16_t len) {
    CAT24C256_ReadBuffer(addr, (uint8_t *)data, len);
    uint8_t stored_crc = 0u;
    CAT24C256_ReadBuffer((uint16_t)(addr + len), &stored_crc, 1u);
    uint8_t calc_crc = eeprom_crc8((const uint8_t *)data, len);
    return (stored_crc == calc_crc);
}

/* -------------------------------------------------------------------------- */
/*  Magic sentinel                                                             */
/* -------------------------------------------------------------------------- */

bool eeprom_check_magic(void) {
    uint8_t buf[2];
    CAT24C256_ReadBuffer(EEPROM_MAGIC_ADDR, buf, 2u);
    uint16_t magic = ((uint16_t)buf[0] << 8) | buf[1];
    return (magic == EEPROM_MAGIC_VALUE);
}

void eeprom_write_magic(void) {
    uint8_t buf[2] = {
        (uint8_t)((EEPROM_MAGIC_VALUE >> 8) & 0xFFu),
        (uint8_t)(EEPROM_MAGIC_VALUE & 0xFFu)
    };
    CAT24C256_WriteBuffer(EEPROM_MAGIC_ADDR, buf, 2u);
}

/* -------------------------------------------------------------------------- */
/*  Raw access                                                                 */
/* -------------------------------------------------------------------------- */

void eeprom_read_raw(uint16_t addr, uint8_t *buf, uint16_t len) {
    CAT24C256_ReadBuffer(addr, buf, len);
}

/* -------------------------------------------------------------------------- */
/*  Erase                                                                      */
/* -------------------------------------------------------------------------- */

void eeprom_erase(void) {
    uint8_t page[EEPROM_PAGE_SIZE];
    memset(page, 0xFFu, sizeof(page));

    uint32_t num_pages = EEPROM_TOTAL_SIZE / EEPROM_PAGE_SIZE;  /* 512 pages */
    INFO_PRINT("%s Erasing %u pages (≈2.5 s)...\r\n", EEPROM_TAG, (unsigned)num_pages);

    for (uint32_t p = 0u; p < num_pages; p++) {
        CAT24C256_WriteBuffer((uint16_t)(p * EEPROM_PAGE_SIZE), page, EEPROM_PAGE_SIZE);
        /* CAT24C256_WriteBuffer already yields 5 ms per page via vTaskDelay */
    }

    INFO_PRINT("%s Erase complete\r\n", EEPROM_TAG);
}
