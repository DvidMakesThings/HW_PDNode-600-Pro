/**
 * @file drivers/snmp_table.c
 * @brief PDNode-600 Pro SNMP MIB table — minimal RFC1213 system group.
 *
 * Provides the required SNMP agent symbols:
 *   snmpData, maxData, COMMUNITY, COMMUNITY_SIZE, initTable(), initial_Trap()
 *
 * @project PDNode-600 Pro
 * @version 1.0.0
 */

#include "../CONFIG.h"
#include "snmp.h"
#include "ethernet_driver.h"

/* --------------------------------------------------------------------------
 *  Community string
 * -------------------------------------------------------------------------- */
const uint8_t COMMUNITY[]    = "public";
const uint8_t COMMUNITY_SIZE = (uint8_t)(sizeof(COMMUNITY) - 1);

/* --------------------------------------------------------------------------
 *  System group getters  (RFC1213 .1.3.6.1.2.1.1.X.0)
 * -------------------------------------------------------------------------- */

static void get_sysDescr(void *buf, uint8_t *len) {
    const char *s = "PDNode-600 Pro Managed USB-C PDU";
    uint8_t L = (uint8_t)strlen(s);
    memcpy(buf, s, L);
    *len = L;
}

static void get_sysName(void *buf, uint8_t *len) {
    const char *s = "PDNode-600 Pro";
    uint8_t L = (uint8_t)strlen(s);
    memcpy(buf, s, L);
    *len = L;
}

static void get_sysContact(void *buf, uint8_t *len) {
    const char *s = "admin@pdnode.local";
    uint8_t L = (uint8_t)strlen(s);
    memcpy(buf, s, L);
    *len = L;
}

static void get_sysLocation(void *buf, uint8_t *len) {
    const char *s = "Server Room";
    uint8_t L = (uint8_t)strlen(s);
    memcpy(buf, s, L);
    *len = L;
}

/* --------------------------------------------------------------------------
 *  MIB table
 * -------------------------------------------------------------------------- */

/* clang-format off */
snmp_entry_t snmpData[] = {
    /* sysDescr    .1.3.6.1.2.1.1.1.0 */
    {8, {0x2b,6,1,2,1,1,1,0}, SNMPDTYPE_OCTET_STRING, 0, {""}, get_sysDescr,    NULL},
    /* sysObjectID .1.3.6.1.2.1.1.2.0 — enterprise 1.3.6.1.4.1.99999 (private) */
    {8, {0x2b,6,1,2,1,1,2,0}, SNMPDTYPE_OBJ_ID,       8, {"\x2b\x06\x01\x04\x01\x86\x9f\x1f"}, NULL, NULL},
    /* sysUpTime   .1.3.6.1.2.1.1.3.0 */
    {8, {0x2b,6,1,2,1,1,3,0}, SNMPDTYPE_TIME_TICKS,   4, {""}, SNMP_GetUptime,  NULL},
    /* sysContact  .1.3.6.1.2.1.1.4.0 */
    {8, {0x2b,6,1,2,1,1,4,0}, SNMPDTYPE_OCTET_STRING, 0, {""}, get_sysContact,  NULL},
    /* sysName     .1.3.6.1.2.1.1.5.0 */
    {8, {0x2b,6,1,2,1,1,5,0}, SNMPDTYPE_OCTET_STRING, 0, {""}, get_sysName,     NULL},
    /* sysLocation .1.3.6.1.2.1.1.6.0 */
    {8, {0x2b,6,1,2,1,1,6,0}, SNMPDTYPE_OCTET_STRING, 0, {""}, get_sysLocation, NULL},
    /* sysServices .1.3.6.1.2.1.1.7.0 — value 72 (end-to-end layer) */
    {8, {0x2b,6,1,2,1,1,7,0}, SNMPDTYPE_INTEGER,      4, { .intval = 72 },       NULL, NULL},
};
/* clang-format on */

const int32_t maxData = (int32_t)(sizeof(snmpData) / sizeof(snmpData[0]));

/* --------------------------------------------------------------------------
 *  Required callbacks
 * -------------------------------------------------------------------------- */

void initTable(void) {
    /* Table is statically initialised — nothing to do. */
}

void initial_Trap(uint8_t *managerIP, uint8_t *agentIP) {
    /* No traps configured at this revision. */
    (void)managerIP;
    (void)agentIP;
}
