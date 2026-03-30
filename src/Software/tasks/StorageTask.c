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
    .location = DEFAULT_LOCATION,
    .serial   = DEFAULT_SERIAL
};

/* -------------------------------------------------------------------------- */
/*  Runtime state                                                             */
/* -------------------------------------------------------------------------- */
typedef enum {
    WRITE_NONE  = 0,
    WRITE_NET   = (1 << 0),
    WRITE_IDENT = (1 << 1)
} write_flags_t;

static volatile bool        s_ready           = false;
static pdnode_net_cfg_t     s_net_cache;
static pdnode_identity_t    s_ident_cache;
static SemaphoreHandle_t    s_cache_mutex     = NULL;
static volatile uint8_t     s_pending         = WRITE_NONE;
static volatile uint32_t    s_dirty_ts        = 0;
static volatile uint32_t    s_prov_unlock_ms  = 0;  /* 0 = locked */

/* -------------------------------------------------------------------------- */
/*  CRC-8 and MAC helpers                                                     */
/* -------------------------------------------------------------------------- */

static uint8_t calc_crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++)
            crc = (crc & 0x80u) ? (uint8_t)((crc << 1) ^ 0x07u) : (uint8_t)(crc << 1);
    }
    return crc;
}

/** FNV-1a hash of serial string → 3-byte suffix, prefixed with "PD" OUI. */
static void pdnode_fill_mac(const char *serial, uint8_t mac[6]) {
    uint32_t h = 0x811C9DC5u;
    const char *s = serial;
    while (*s) { h ^= (uint8_t)*s++; h *= 0x01000193u; }
    mac[0] = PDNODE_MAC_PREFIX0;
    mac[1] = PDNODE_MAC_PREFIX1;
    mac[2] = PDNODE_MAC_PREFIX2;
    mac[3] = (uint8_t)(h >> 16);
    mac[4] = (uint8_t)(h >> 8);
    mac[5] = (uint8_t)h;
}

static bool pdnode_mac_needs_repair(const uint8_t mac[6]) {
    bool wrong_pfx  = (mac[0] != PDNODE_MAC_PREFIX0 ||
                       mac[1] != PDNODE_MAC_PREFIX1 ||
                       mac[2] != PDNODE_MAC_PREFIX2);
    bool zero_sfx   = (mac[3] | mac[4] | mac[5]) == 0x00u;
    bool ff_sfx     = (mac[3] & mac[4] & mac[5]) == 0xFFu;
    return wrong_pfx || zero_sfx || ff_sfx;
}

/* -------------------------------------------------------------------------- */
/*  Provisioning helpers                                                       */
/* -------------------------------------------------------------------------- */

/** Convert ASCII string to lowercase hex string ("admin" → "61646d696e"). */
static void ascii_to_hex_lower(const char *in, char *out, size_t out_max) {
    const char *hex = "0123456789abcdef";
    size_t i;
    for (i = 0; in[i] != '\0' && (i * 2u + 2u) < out_max; i++) {
        out[i * 2u]      = hex[(unsigned char)in[i] >> 4];
        out[i * 2u + 1u] = hex[(unsigned char)in[i] & 0x0Fu];
    }
    out[i * 2u] = '\0';
}

static bool prov_window_open(void) {
    if (s_prov_unlock_ms == 0u) return false;
    uint32_t elapsed = to_ms_since_boot(get_absolute_time()) - s_prov_unlock_ms;
    if (elapsed > PROV_UNLOCK_TIMEOUT_MS) {
        s_prov_unlock_ms = 0u;   /* auto-expire */
        return false;
    }
    return true;
}

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

static void net_write_with_crc(const pdnode_net_cfg_t *cfg) {
    CAT24C256_WriteBuffer(STORAGE_NET_ADDR, (const uint8_t *)cfg, sizeof(*cfg));
    uint8_t crc = calc_crc8((const uint8_t *)cfg, sizeof(*cfg));
    CAT24C256_WriteBuffer(STORAGE_NET_CRC_ADDR, &crc, 1);
}

static void load_defaults(void) {
    INFO_PRINT("%s EEPROM blank — writing defaults\r\n", STORAGE_TAG);
    s_net_cache   = DEFAULT_NET;
    s_ident_cache = DEFAULT_IDENTITY;
    /* Derive MAC from default serial */
    pdnode_fill_mac(s_ident_cache.serial, s_net_cache.mac);
    INFO_PRINT("%s MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n", STORAGE_TAG,
               s_net_cache.mac[0], s_net_cache.mac[1], s_net_cache.mac[2],
               s_net_cache.mac[3], s_net_cache.mac[4], s_net_cache.mac[5]);
    eeprom_write_magic();
    net_write_with_crc(&s_net_cache);
    CAT24C256_WriteBuffer(STORAGE_IDENTITY_ADDR,
                          (const uint8_t *)&s_ident_cache, sizeof(s_ident_cache));
}

static void load_from_eeprom(void) {
    /* Identity first — serial is needed for MAC validation */
    CAT24C256_ReadBuffer(STORAGE_IDENTITY_ADDR,
                         (uint8_t *)&s_ident_cache, sizeof(s_ident_cache));
    s_ident_cache.serial[sizeof(s_ident_cache.serial) - 1] = '\0';

    /* Net config with CRC check */
    CAT24C256_ReadBuffer(STORAGE_NET_ADDR,
                         (uint8_t *)&s_net_cache, sizeof(s_net_cache));
    uint8_t stored_crc = 0;
    CAT24C256_ReadBuffer(STORAGE_NET_CRC_ADDR, &stored_crc, 1);
    uint8_t calc_crc = calc_crc8((const uint8_t *)&s_net_cache, sizeof(s_net_cache));

    if (stored_crc != calc_crc) {
        WARNING_PRINT("%s Net CRC mismatch (0x%02X vs 0x%02X) — restoring defaults\r\n",
                      STORAGE_TAG, stored_crc, calc_crc);
        s_net_cache = DEFAULT_NET;
        pdnode_fill_mac(s_ident_cache.serial, s_net_cache.mac);
        net_write_with_crc(&s_net_cache);
    } else if (pdnode_mac_needs_repair(s_net_cache.mac)) {
        pdnode_fill_mac(s_ident_cache.serial, s_net_cache.mac);
        net_write_with_crc(&s_net_cache);
        INFO_PRINT("%s MAC repaired from serial\r\n", STORAGE_TAG);
    }

    INFO_PRINT("%s Config loaded. IP: %u.%u.%u.%u  Serial: %s\r\n", STORAGE_TAG,
               s_net_cache.ip[0], s_net_cache.ip[1],
               s_net_cache.ip[2], s_net_cache.ip[3],
               s_ident_cache.serial);
}

static void flush_pending(void) {
    if (s_pending & WRITE_NET) {
        net_write_with_crc(&s_net_cache);
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

bool Storage_GetSerial(char serial[16]) {
    if (!serial || !s_ready) return false;
    if (xSemaphoreTake(s_cache_mutex, pdMS_TO_TICKS(50))) {
        memcpy(serial, s_ident_cache.serial, 16);
        xSemaphoreGive(s_cache_mutex);
        return true;
    }
    return false;
}

void Storage_FillMac(uint8_t mac[6]) {
    if (!mac) return;
    if (xSemaphoreTake(s_cache_mutex, pdMS_TO_TICKS(50))) {
        pdnode_fill_mac(s_ident_cache.serial, mac);
        xSemaphoreGive(s_cache_mutex);
    }
}

bool Storage_ProvUnlock(const char *passcode) {
    if (!passcode || !*passcode) return false;
    char hex[64] = {0};
    ascii_to_hex_lower(passcode, hex, sizeof(hex));
    if (strcmp(hex, PROV_UNLOCK_TOKEN) != 0) {
        INFO_PRINT("%s Unlock failed: bad passcode\r\n", STORAGE_TAG);
        return false;
    }
    s_prov_unlock_ms = to_ms_since_boot(get_absolute_time());
    INFO_PRINT("%s Provisioning UNLOCKED for %u s\r\n",
               STORAGE_TAG, (unsigned)(PROV_UNLOCK_TIMEOUT_MS / 1000u));
    return true;
}

bool Storage_ProvIsUnlocked(void) { return prov_window_open(); }

void Storage_ProvLock(void) {
    s_prov_unlock_ms = 0u;
    INFO_PRINT("%s Provisioning LOCKED\r\n", STORAGE_TAG);
}

int Storage_SetSerial(const char *serial) {
    if (!prov_window_open()) {
        INFO_PRINT("%s Set serial rejected: locked\r\n", STORAGE_TAG);
        return -1;
    }
    if (!serial || !*serial) return -2;
    size_t len = strlen(serial);
    if (len > 15u) return -2;
    for (size_t i = 0u; i < len; i++) {
        char c = serial[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '-'))
            return -2;
    }
    if (!xSemaphoreTake(s_cache_mutex, pdMS_TO_TICKS(100))) return -3;
    strncpy(s_ident_cache.serial, serial, sizeof(s_ident_cache.serial) - 1u);
    s_ident_cache.serial[sizeof(s_ident_cache.serial) - 1u] = '\0';
    pdnode_fill_mac(s_ident_cache.serial, s_net_cache.mac);
    s_pending  |= (uint8_t)(WRITE_NET | WRITE_IDENT);
    s_dirty_ts  = to_ms_since_boot(get_absolute_time());
    xSemaphoreGive(s_cache_mutex);
    INFO_PRINT("%s Serial set: %s  MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
               STORAGE_TAG, serial,
               s_net_cache.mac[0], s_net_cache.mac[1], s_net_cache.mac[2],
               s_net_cache.mac[3], s_net_cache.mac[4], s_net_cache.mac[5]);
    return 0;
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
