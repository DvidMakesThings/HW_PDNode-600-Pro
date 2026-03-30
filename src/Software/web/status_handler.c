/**
 * @file web/status_handler.c
 * @brief GET /api/status — full system telemetry JSON for the web dashboard.
 *
 * Aggregates data from PDCardTask and USBATask, plus Ethernet info from W5500,
 * and serialises to JSON that matches the model schema in index.html.
 *
 * @project PDNode-600 Pro
 * @version 1.0.0
 */

#include "../CONFIG.h"
#include "status_handler.h"
#include "../tasks/HealthTask.h"
#include "../tasks/PDCardTask.h"
#include "../tasks/USBATask.h"
#include "../drivers/PAC1720_driver.h"
#include "../drivers/ethernet_driver.h"
#include "../drivers/socket.h"

#define STATUS_TAG  "[STATUS]"

/* Boot time reference (set once on first call) */
static uint32_t s_boot_ms = 0;

static void send_json(uint8_t sock, const char *json, int len) {
    char hdr[160];
    int hlen = snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Cache-Control: no-cache\r\n"
        "Connection: close\r\n\r\n",
        len);
    Health_Heartbeat(HEALTH_ID_NET);
    send(sock, (uint8_t *)hdr, (uint16_t)hlen);
    Health_Heartbeat(HEALTH_ID_NET);
    send(sock, (uint8_t *)json, (uint16_t)len);
    Health_Heartbeat(HEALTH_ID_NET);
}

void handle_status_request(uint8_t sock) {
    NETLOG_PRINT(">> handle_status_request\r\n");

    if (s_boot_ms == 0) s_boot_ms = to_ms_since_boot(get_absolute_time());
    uint32_t uptime_s = (to_ms_since_boot(get_absolute_time()) - s_boot_ms) / 1000;

    /* Read W5500 IP and link state */
    uint8_t ip[4] = {0};
    getSIPR(ip);
    w5500_PhyLink link = w5500_get_link_status();
    const char *link_str = (link == PHY_LINK_ON) ? "100M Full" : "No Link";

    /* Allocate JSON buffer — 8 KB covers all PD + USB-A fields with margin */
    const int cap = 8192;
    char *json = pvPortMalloc(cap);
    if (!json) {
        const char *err = "{\"error\":\"oom\"}";
        send_json(sock, err, (int)strlen(err));
        return;
    }

    int pos = 0;
#define JSON_APPEND(...) \
    do { \
        if (pos < cap - 1) \
            pos += snprintf(json + pos, cap - pos, __VA_ARGS__); \
    } while (0)

    /* ---- sys object ---- */
    JSON_APPEND("{\"sys\":{"
        "\"ip\":\"%u.%u.%u.%u\","
        "\"link\":\"%s\","
        "\"uptime\":%lu,"
        "\"fw\":\"%s\","
        "\"hw\":\"%s\""
        "},",
        ip[0], ip[1], ip[2], ip[3],
        link_str,
        (unsigned long)uptime_s,
        FIRMWARE_VERSION,
        HARDWARE_VERSION);

    Health_Heartbeat(HEALTH_ID_NET);

    /* ---- pd array (8 PDCard ports) ---- */
    JSON_APPEND("\"pd\":[");

    for (int p = 0; p < PDCARD_NUM_PORTS; p++) {
        pdcard_telemetry_t t;
        bool ok = PDCard_GetTelemetry((uint8_t)p, &t);
        if (!ok) {
            memset(&t, 0, sizeof(t));
            t.connected = false;
            strncpy(t.port_state, "Disconnected", sizeof(t.port_state));
            strncpy(t.contract,   "None",         sizeof(t.contract));
        }

        JSON_APPEND("{"
            "\"id\":%d,"
            "\"name\":\"PD %d\","
            "\"alias\":\"Port %d\","
            "\"enabled\":%s,"
            "\"voltage\":%.3f,"
            "\"current\":%.3f,"
            "\"uptime\":%lu,"
            "\"portState\":\"%s\","
            "\"contract\":\"%s\","
            "\"role\":\"Source\","
            "\"dataRole\":\"%s\","
            "\"cablePresent\":%s,"
            "\"emarked\":false,"
            "\"cableCap\":\"N/A\","
            "\"vbusPresent\":%s,"
            "\"vbusMatch\":%s,"
            "\"discharge\":false,"
            "\"vconnOn\":%s,"
            "\"vconnFault\":false,"
            "\"fault\":\"None\","
            "\"ocShort\":false,"
            "\"vbusErr\":false,"
            "\"wdtReset\":false,"
            "\"retries\":0,"
            "\"lastFault\":\"None\","
            "\"rxBadCrc\":0,"
            "\"rxDup\":0,"
            "\"lastMsgSec\":null,"
            "\"lastGoodCrcSec\":null,"
            "\"lastPdoList\":\"N/A\""
            "}%s",
            p + 1, p + 1, p + 1,
            t.connected ? "true" : "false",
            t.vbus_v,
            t.current_a,
            (unsigned long)t.uptime_s,
            t.port_state,
            t.contract,
            t.pd_active ? "DFP" : "N/A",
            t.connected ? "true" : "false",
            t.connected ? "true" : "false",
            t.connected ? "true" : "false",
            t.connected ? "true" : "false",
            (p < PDCARD_NUM_PORTS - 1) ? "," : "");

        if (p % 2 == 0) Health_Heartbeat(HEALTH_ID_NET);
    }
    JSON_APPEND("],");

    /* ---- usbA array (4 USB-A ports) ---- */
    JSON_APPEND("\"usbA\":[");

    for (int p = 0; p < USBA_NUM_PORTS; p++) {
        usba_telemetry_t t;
        bool ok = USBA_GetTelemetry((uint8_t)p, &t);
        if (!ok) {
            memset(&t, 0, sizeof(t));
        }

        JSON_APPEND("{"
            "\"id\":%d,"
            "\"name\":\"USB-A %d\","
            "\"enabled\":%s,"
            "\"voltage\":%.3f,"
            "\"current\":%.3f,"
            "\"uptime\":%lu,"
            "\"enablePin\":%s,"
            "\"faultPin\":%s,"
            "\"currentLimit\":\"1.0 A\","
            "\"ina\":{"
            "\"busV\":%.3f,"
            "\"shuntmV\":%.2f,"
            "\"powerW\":%.3f"
            "}"
            "}%s",
            p + 1, p + 1,
            t.enabled ? "true" : "false",
            t.voltage_v,
            t.current_a,
            (unsigned long)t.uptime_s,
            t.enabled ? "true" : "false",
            t.fault   ? "true" : "false",
            t.voltage_v,
            t.current_a * PAC1720_RSENSE_OHM * 1000.0f, /* shuntmV = I * R * 1000 */
            t.power_w,
            (p < USBA_NUM_PORTS - 1) ? "," : "");

        Health_Heartbeat(HEALTH_ID_NET);
    }
    JSON_APPEND("]}");

    send_json(sock, json, pos);
    vPortFree(json);

    NETLOG_PRINT("<< handle_status_request done (%d bytes)\r\n", pos);
}
