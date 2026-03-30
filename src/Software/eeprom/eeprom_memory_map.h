/**
 * @file eeprom/eeprom_memory_map.h
 * @brief CAT24C256 EEPROM layout for PDNode-600 Pro.
 *
 * Memory map (all blocks: [data bytes][CRC-8 byte]):
 *
 *  0x0000  2 B   Magic sentinel (no CRC — value itself is the check)
 *  0x0010 24 B   Network config  (23 B pdnode_net_cfg_t  + 1 B CRC)
 *  0x0060 81 B   Device identity (80 B pdnode_identity_t + 1 B CRC)
 *  0x00B1        Reserved / future use
 *  0x7FFF        End of EEPROM (32 KB)
 *
 * Bump EEPROM_MAGIC_VALUE whenever a stored struct layout changes.
 * This forces EEPROM re-initialisation with factory defaults on the
 * next boot, avoiding misinterpretation of old binary data.
 *
 * @project PDNode-600 Pro
 * @version 1.0.0
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>

/* -------------------------------------------------------------------------- */
/*  EEPROM geometry                                                            */
/* -------------------------------------------------------------------------- */
#define EEPROM_TOTAL_SIZE       32768u   /* 32 KB */
#define EEPROM_PAGE_SIZE        64u      /* bytes per write page */

/* -------------------------------------------------------------------------- */
/*  Block addresses                                                            */
/* -------------------------------------------------------------------------- */

/** Magic sentinel — 2 bytes, checked on boot to detect blank/incompatible EEPROM. */
#define EEPROM_MAGIC_ADDR       0x0000u
#define EEPROM_MAGIC_VALUE      0xA5C5u  /* bump: added CRC to identity block */

/** Network configuration block base address. */
#define EEPROM_NET_ADDR         0x0010u

/** Device identity block base address. */
#define EEPROM_IDENTITY_ADDR    0x0060u

/* -------------------------------------------------------------------------- */
/*  Stored structures                                                          */
/* -------------------------------------------------------------------------- */

/** Network configuration persisted in EEPROM (23 bytes + 1 CRC). */
typedef struct {
    uint8_t ip[4];
    uint8_t sn[4];
    uint8_t gw[4];
    uint8_t dns[4];
    uint8_t mac[6];
    uint8_t dhcp;   /* 0 = static, 1 = DHCP */
} pdnode_net_cfg_t;

/** Device identity persisted in EEPROM (80 bytes + 1 CRC). */
typedef struct {
    char name[32];
    char location[32];
    char serial[16];  /* factory-provisioned serial number */
} pdnode_identity_t;
