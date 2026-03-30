/**
 * @file tasks/StorageTask.h
 * @brief Non-volatile configuration storage over AT24C256 EEPROM.
 *
 * StorageTask owns all EEPROM access. Other tasks call the public API
 * functions which either read from a RAM cache or queue a write.
 *
 * EEPROM layout (addresses):
 *  0x0000 - 0x000F : Magic + version header
 *  0x0010 - 0x005F : Network configuration (IP, MAC, subnet, GW, DNS, DHCP)
 *  0x0060 - 0x00BF : Device identity (name, location)
 *  0x00C0 - 0x00FF : Reserved
 *
 * @project PDNode-600 Pro
 */

#pragma once
#include "../CONFIG.h"
#include "../drivers/ethernet_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/*  Stored structures                                                         */
/* -------------------------------------------------------------------------- */

/** Network configuration persisted in EEPROM. */
typedef struct {
    uint8_t ip[4];
    uint8_t sn[4];
    uint8_t gw[4];
    uint8_t dns[4];
    uint8_t mac[6];
    uint8_t dhcp;   /* 0 = static, 1 = DHCP */
} pdnode_net_cfg_t;

/** Device identity persisted in EEPROM. */
typedef struct {
    char name[32];
    char location[32];
} pdnode_identity_t;

/* -------------------------------------------------------------------------- */
/*  EEPROM layout constants                                                   */
/* -------------------------------------------------------------------------- */
#define STORAGE_MAGIC_ADDR      0x0000u
#define STORAGE_MAGIC_VALUE     0xA5C3u
#define STORAGE_NET_ADDR        0x0010u
#define STORAGE_IDENTITY_ADDR   0x0060u

/* -------------------------------------------------------------------------- */
/*  Public API                                                                */
/* -------------------------------------------------------------------------- */

/** Create and start StorageTask. */
BaseType_t StorageTask_Init(bool enable);

/** Returns true when config has been loaded from EEPROM (or defaults applied). */
bool Storage_IsReady(void);

/** Wait (blocking) until storage is ready or timeout_ms elapses. */
bool Storage_WaitReady(uint32_t timeout_ms);

/** Get network config from RAM cache. */
bool Storage_GetNetConfig(pdnode_net_cfg_t *out);

/** Save network config to EEPROM (async, queued). */
void Storage_SetNetConfig(const pdnode_net_cfg_t *cfg);

/** Get device identity from RAM cache. */
bool Storage_GetIdentity(pdnode_identity_t *out);

/** Save device identity to EEPROM (async, queued). */
void Storage_SetIdentity(const pdnode_identity_t *id);

/** Fill a w5500_NetConfig from storage (for use by NetTask). */
bool Storage_FillEthConfig(w5500_NetConfig *eth);

#ifdef __cplusplus
}
#endif
