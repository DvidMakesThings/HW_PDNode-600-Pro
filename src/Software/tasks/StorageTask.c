/**
 * @file tasks/StorageTask.c
 * @brief Non-volatile config owner — AT24C256 EEPROM over I2C0.
 *
 * Owns all EEPROM access. Maintains a RAM cache for fast reads.
 * Writes are debounced: a 2 s idle timeout fires before the EEPROM write.
 *
 * @project PDNode-600 Pro
 * @version 1.0.0
 */

#include "../CONFIG.h"
#include "StorageTask.h"
#include "HealthTask.h"
#include "../drivers/CAT24C256_driver.h"
#include "../drivers/i2c_bus.h"

#define STORAGE_TAG         "[STORAGE]"
#define STORAGE_STACK       2048
#define STORAGE_DEBOUNCE_MS 2000

/* -------------------------------------------------------------------------- */
/*  Default values                                                            */
/* -------------------------------------------------------------------------- */
static const pdnode_net_cfg_t DEFAULT_NET = {
    .ip   = DEFAULT_IP,
    .sn   = DEFAULT_SUBNET,
    .gw   = DEFAULT_GW,
    .dns  = DEFAULT_DNS,
    .mac  = DEFAULT_MAC,
    .dhcp = DEFAULT_DHCP
};

static const pdnode_identity_t DEFAULT_IDENTITY = {
    .name     = DEFAULT_DEVICE_NAME,
    .location = DEFAULT_LOCATION
};

/* -------------------------------------------------------------------------- */
/*  Runtime state                                                             */
/* -------------------------------------------------------------------------- */
typedef enum {
    WRITE_NONE  = 0,
    WRITE_NET   = (1 << 0),
    WRITE_IDENT = (1 << 1)
} write_flags_t;

static volatile bool        s_ready        = false;
static pdnode_net_cfg_t     s_net_cache;
static pdnode_identity_t    s_ident_cache;
static SemaphoreHandle_t    s_cache_mutex  = NULL;
static volatile uint8_t     s_pending      = WRITE_NONE;
static volatile uint32_t    s_dirty_ts     = 0;

/* -------------------------------------------------------------------------- */
/*  EEPROM helpers                                                            */
/* -------------------------------------------------------------------------- */

static bool eeprom_valid(void) {
    uint8_t buf[2];
    CAT24C256_ReadBuffer(STORAGE_MAGIC_ADDR, buf, 2);
    uint16_t magic = ((uint16_t)buf[0] << 8) | buf[1];
    return (magic == STORAGE_MAGIC_VALUE);
}

static void eeprom_write_magic(void) {
    uint8_t buf[2] = {(STORAGE_MAGIC_VALUE >> 8) & 0xFF,
                       STORAGE_MAGIC_VALUE & 0xFF};
    CAT24C256_WriteBuffer(STORAGE_MAGIC_ADDR, buf, 2);
}

static void load_defaults(void) {
    INFO_PRINT("%s EEPROM blank — writing defaults\r\n", STORAGE_TAG);
    s_net_cache   = DEFAULT_NET;
    s_ident_cache = DEFAULT_IDENTITY;
    eeprom_write_magic();
    CAT24C256_WriteBuffer(STORAGE_NET_ADDR,
                          (const uint8_t *)&s_net_cache, sizeof(s_net_cache));
    CAT24C256_WriteBuffer(STORAGE_IDENTITY_ADDR,
                          (const uint8_t *)&s_ident_cache, sizeof(s_ident_cache));
}

static void load_from_eeprom(void) {
    CAT24C256_ReadBuffer(STORAGE_NET_ADDR,
                         (uint8_t *)&s_net_cache, sizeof(s_net_cache));
    CAT24C256_ReadBuffer(STORAGE_IDENTITY_ADDR,
                         (uint8_t *)&s_ident_cache, sizeof(s_ident_cache));
    INFO_PRINT("%s Config loaded. IP: %u.%u.%u.%u\r\n", STORAGE_TAG,
               s_net_cache.ip[0], s_net_cache.ip[1],
               s_net_cache.ip[2], s_net_cache.ip[3]);
}

static void flush_pending(void) {
    if (s_pending & WRITE_NET) {
        CAT24C256_WriteBuffer(STORAGE_NET_ADDR,
                              (const uint8_t *)&s_net_cache, sizeof(s_net_cache));
        INFO_PRINT("%s Net config written to EEPROM\r\n", STORAGE_TAG);
    }
    if (s_pending & WRITE_IDENT) {
        CAT24C256_WriteBuffer(STORAGE_IDENTITY_ADDR,
                              (const uint8_t *)&s_ident_cache, sizeof(s_ident_cache));
        INFO_PRINT("%s Identity written to EEPROM\r\n", STORAGE_TAG);
    }
    s_pending = WRITE_NONE;
}

/* -------------------------------------------------------------------------- */
/*  Task function                                                             */
/* -------------------------------------------------------------------------- */

static void StorageTask_Function(void *arg) {
    (void)arg;
    INFO_PRINT("%s Task started\r\n", STORAGE_TAG);

    CAT24C256_Init();

    if (eeprom_valid()) {
        load_from_eeprom();
    } else {
        load_defaults();
    }

    s_ready = true;
    INFO_PRINT("%s Ready\r\n", STORAGE_TAG);

    for (;;) {
        Health_Heartbeat(HEALTH_ID_STORAGE);

        /* Debounced write: flush if pending writes are idle for DEBOUNCE ms */
        if (s_pending != WRITE_NONE) {
            uint32_t now_ms = to_ms_since_boot(get_absolute_time());
            if ((now_ms - s_dirty_ts) >= STORAGE_DEBOUNCE_MS) {
                if (xSemaphoreTake(s_cache_mutex, pdMS_TO_TICKS(100))) {
                    flush_pending();
                    xSemaphoreGive(s_cache_mutex);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

/* -------------------------------------------------------------------------- */
/*  Public API                                                                */
/* -------------------------------------------------------------------------- */

BaseType_t StorageTask_Init(bool enable) {
    if (!enable) return pdPASS;

    s_cache_mutex = xSemaphoreCreateMutex();
    if (!s_cache_mutex) return pdFAIL;

    return xTaskCreate(StorageTask_Function, "Storage", STORAGE_STACK,
                       NULL, STORAGETASK_PRIORITY, NULL);
}

bool Storage_IsReady(void) { return s_ready; }

bool Storage_WaitReady(uint32_t timeout_ms) {
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
    while (!s_ready && xTaskGetTickCount() < deadline) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return s_ready;
}

bool Storage_GetNetConfig(pdnode_net_cfg_t *out) {
    if (!out || !s_ready) return false;
    if (xSemaphoreTake(s_cache_mutex, pdMS_TO_TICKS(50))) {
        *out = s_net_cache;
        xSemaphoreGive(s_cache_mutex);
        return true;
    }
    return false;
}

void Storage_SetNetConfig(const pdnode_net_cfg_t *cfg) {
    if (!cfg) return;
    if (xSemaphoreTake(s_cache_mutex, pdMS_TO_TICKS(50))) {
        s_net_cache = *cfg;
        s_pending  |= WRITE_NET;
        s_dirty_ts  = to_ms_since_boot(get_absolute_time());
        xSemaphoreGive(s_cache_mutex);
    }
}

bool Storage_GetIdentity(pdnode_identity_t *out) {
    if (!out || !s_ready) return false;
    if (xSemaphoreTake(s_cache_mutex, pdMS_TO_TICKS(50))) {
        *out = s_ident_cache;
        xSemaphoreGive(s_cache_mutex);
        return true;
    }
    return false;
}

void Storage_SetIdentity(const pdnode_identity_t *id) {
    if (!id) return;
    if (xSemaphoreTake(s_cache_mutex, pdMS_TO_TICKS(50))) {
        s_ident_cache = *id;
        s_pending    |= WRITE_IDENT;
        s_dirty_ts    = to_ms_since_boot(get_absolute_time());
        xSemaphoreGive(s_cache_mutex);
    }
}

bool Storage_FillEthConfig(w5500_NetConfig *eth) {
    if (!eth || !s_ready) return false;
    pdnode_net_cfg_t net;
    if (!Storage_GetNetConfig(&net)) return false;
    memcpy(eth->ip,  net.ip,  4);
    memcpy(eth->sn,  net.sn,  4);
    memcpy(eth->gw,  net.gw,  4);
    memcpy(eth->dns, net.dns, 4);
    memcpy(eth->mac, net.mac, 6);
    eth->dhcp = net.dhcp;
    return true;
}
