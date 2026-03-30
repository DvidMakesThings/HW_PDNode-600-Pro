/**
 * @file tasks/StorageTask.h
 * @brief Non-volatile configuration storage over CAT24C256 EEPROM.
 *
 * StorageTask owns all EEPROM access. Other tasks call the public API
 * which either reads from a RAM cache or queues a debounced write (2 s idle).
 *
 * EEPROM layout and stored struct definitions live in eeprom/eeprom_memory_map.h.
 *
 * @project PDNode-600 Pro
 * @version 1.0.0
 */

#pragma once
#include "../CONFIG.h"
#include "../eeprom/eeprom_memory_map.h"
#include "../drivers/ethernet_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/*  Lifecycle                                                                  */
/* -------------------------------------------------------------------------- */

/** Create and start StorageTask. */
BaseType_t StorageTask_Init(bool enable);

/** Returns true when config has been loaded from EEPROM (or defaults applied). */
bool Storage_IsReady(void);

/** Block until storage is ready or @p timeout_ms elapses. */
bool Storage_WaitReady(uint32_t timeout_ms);

/* -------------------------------------------------------------------------- */
/*  Network config                                                             */
/* -------------------------------------------------------------------------- */

/** Read network config from RAM cache. */
bool Storage_GetNetConfig(pdnode_net_cfg_t *out);

/** Queue network config write to EEPROM (debounced, async). */
void Storage_SetNetConfig(const pdnode_net_cfg_t *cfg);

/* -------------------------------------------------------------------------- */
/*  Device identity                                                            */
/* -------------------------------------------------------------------------- */

/** Read device identity from RAM cache. */
bool Storage_GetIdentity(pdnode_identity_t *out);

/** Queue identity write to EEPROM (debounced, async). */
void Storage_SetIdentity(const pdnode_identity_t *id);

/* -------------------------------------------------------------------------- */
/*  Convenience helpers                                                        */
/* -------------------------------------------------------------------------- */

/** Populate a w5500_NetConfig from stored network config (used by NetTask). */
bool Storage_FillEthConfig(w5500_NetConfig *eth);

/** Copy the stored serial number (always NUL-terminated, max 15 chars). */
bool Storage_GetSerial(char serial[16]);

/** Derive MAC from stored serial via FNV-1a + PD prefix, write into mac[6]. */
void Storage_FillMac(uint8_t mac[6]);

/* -------------------------------------------------------------------------- */
/*  Provisioning API                                                           */
/* -------------------------------------------------------------------------- */

/**
 * Attempt to open the provisioning write window.
 * @param passcode  Plaintext passcode (converted to lowercase hex internally).
 * @return true if unlocked, false on mismatch.
 */
bool Storage_ProvUnlock(const char *passcode);

/** Returns true if the provisioning window is currently open. */
bool Storage_ProvIsUnlocked(void);

/** Force-close the provisioning window. */
void Storage_ProvLock(void);

/**
 * Set device serial number (requires provisioning unlock).
 * Automatically regenerates the MAC address from the new serial.
 * @return 0 ok, -1 locked, -2 invalid (max 15 chars, A-Z 0-9 -), -3 mutex error.
 */
int Storage_SetSerial(const char *serial);

#ifdef __cplusplus
}
#endif
