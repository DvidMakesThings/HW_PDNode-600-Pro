/**
 * @file eeprom/eeprom_io.h
 * @brief Unified EEPROM block I/O with CRC-8 integrity.
 *
 * Every data block is written as [data bytes][CRC-8 byte] and read back
 * with CRC validation. A single write/read API pair handles all block types;
 * callers pass the base address and data size from eeprom_memory_map.h.
 *
 * RTOS-friendly: all write operations delegate to CAT24C256_WriteBuffer which
 * yields via vTaskDelay after each 64-byte page. eeprom_erase() yields between
 * every page write (512 pages × 5 ms ≈ 2.5 s total, non-blocking).
 *
 * @project PDNode-600 Pro
 * @version 1.0.0
 */

#pragma once
#include "eeprom_memory_map.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/*  Initialisation                                                             */
/* -------------------------------------------------------------------------- */

/** Initialise the underlying EEPROM driver (I2C GPIO setup). */
void eeprom_init(void);

/* -------------------------------------------------------------------------- */
/*  CRC helper                                                                 */
/* -------------------------------------------------------------------------- */

/** CRC-8, polynomial 0x07 (SMBUS). */
uint8_t eeprom_crc8(const uint8_t *data, size_t len);

/* -------------------------------------------------------------------------- */
/*  Block I/O                                                                  */
/* -------------------------------------------------------------------------- */

/**
 * Write a data block and its CRC-8 to EEPROM.
 *
 * Layout at @p addr: [len bytes of data][1 byte CRC-8]
 *
 * @param addr  EEPROM start address of the block (from eeprom_memory_map.h).
 * @param data  Pointer to data buffer.
 * @param len   Number of data bytes (excluding the CRC byte).
 */
void eeprom_write_block(uint16_t addr, const void *data, uint16_t len);

/**
 * Read a data block and validate its CRC-8.
 *
 * @param addr  EEPROM start address of the block.
 * @param data  Destination buffer (must be at least @p len bytes).
 * @param len   Number of data bytes (excluding the CRC byte).
 * @return true if CRC matches, false on mismatch (data is still populated).
 */
bool eeprom_read_block(uint16_t addr, void *data, uint16_t len);

/* -------------------------------------------------------------------------- */
/*  Magic sentinel                                                             */
/* -------------------------------------------------------------------------- */

/** Returns true if the EEPROM magic matches EEPROM_MAGIC_VALUE. */
bool eeprom_check_magic(void);

/** Write EEPROM_MAGIC_VALUE to EEPROM_MAGIC_ADDR. */
void eeprom_write_magic(void);

/* -------------------------------------------------------------------------- */
/*  Raw access (for diagnostic tools)                                         */
/* -------------------------------------------------------------------------- */

/**
 * Read raw bytes from EEPROM without CRC validation.
 * Intended for diagnostic dump commands only.
 */
void eeprom_read_raw(uint16_t addr, uint8_t *buf, uint16_t len);

/* -------------------------------------------------------------------------- */
/*  Erase                                                                      */
/* -------------------------------------------------------------------------- */

/**
 * Erase the entire EEPROM by writing 0xFF to all 512 pages.
 * RTOS-friendly: each 64-byte page write yields via vTaskDelay (5 ms).
 * Total duration ≈ 2.5 s. The device should be rebooted after erase.
 */
void eeprom_erase(void);

#ifdef __cplusplus
}
#endif
