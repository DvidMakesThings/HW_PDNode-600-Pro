/**
 * @file tasks/NetTask.c
 * @brief W5500 Ethernet stack — HTTP web UI + SNMP agent.
 *
 * Sequence:
 *  1) Wait for StorageTask to be ready.
 *  2) Read network config from storage, fill w5500_NetConfig.
 *  3) Init W5500 hardware (SPI already running from InitTask).
 *  4) Apply network config + bring up chip.
 *  5) Start HTTP server (port 80) and SNMP agent (port 161).
 *  6) Loop: poll HTTP, poll SNMP, supervise PHY link.
 *
 * @project PDNode-600 Pro
 * @version 1.0.0
 */

#include "../CONFIG.h"
#include "NetTask.h"
#include "HealthTask.h"
#include "StorageTask.h"
#include "../drivers/ethernet_driver.h"
#include "../drivers/ethernet_config.h"
#include "../drivers/socket.h"
#include "../drivers/snmp.h"
#include "../web/http_server.h"

#define NET_TAG         "[NET]"
#define NET_STACK       4096

#define HTTP_SOCKET_NUM 0u
#define SNMP_SOCKET_NUM 1u
#define TRAP_SOCKET_NUM 2u

static TaskHandle_t  s_net_task   = NULL;
static volatile bool s_net_ready  = false;
static w5500_NetConfig s_eth_cfg;
static w5500_PhyLink   s_last_link = PHY_LINK_OFF;

/* -------------------------------------------------------------------------- */

static bool apply_config_and_init(void) {
    pdnode_net_cfg_t net;
    if (!Storage_GetNetConfig(&net)) {
        /* Fallback to hardcoded defaults */
        uint8_t def_ip[4]  = W5500_DEFAULT_IP;
        uint8_t def_sn[4]  = W5500_DEFAULT_SUBNET;
        uint8_t def_gw[4]  = W5500_DEFAULT_GATEWAY;
        uint8_t def_dns[4] = W5500_DEFAULT_DNS;
        uint8_t def_mac[6] = W5500_DEFAULT_MAC;
        memcpy(s_eth_cfg.ip,  def_ip,  4);
        memcpy(s_eth_cfg.sn,  def_sn,  4);
        memcpy(s_eth_cfg.gw,  def_gw,  4);
        memcpy(s_eth_cfg.dns, def_dns, 4);
        memcpy(s_eth_cfg.mac, def_mac, 6);
        s_eth_cfg.dhcp = W5500_USE_DHCP;
    } else {
        Storage_FillEthConfig(&s_eth_cfg);
    }

    if (!w5500_hw_init()) {
        ERROR_PRINT("%s W5500 hw_init failed\r\n", NET_TAG);
        return false;
    }

    if (!w5500_chip_init(&s_eth_cfg)) {
        WARNING_PRINT("%s chip_init failed (no link?)\r\n", NET_TAG);
        /* non-fatal — link supervisor will retry on link-up */
    }

    w5500_print_network(&s_eth_cfg);
    return true;
}

static void start_services(void) {
    http_server_init();
    INFO_PRINT("%s HTTP server on port 80\r\n", NET_TAG);

    if (!SNMP_Init(SNMP_SOCKET_NUM, SNMP_PORT_AGENT, TRAP_SOCKET_NUM)) {
        WARNING_PRINT("%s SNMP init failed\r\n", NET_TAG);
    } else {
        INFO_PRINT("%s SNMP agent on UDP/161\r\n", NET_TAG);
    }
}

static bool reinit_from_cache(void) {
    w5500_sw_reset();
    vTaskDelay(pdMS_TO_TICKS(150));
    if (w5500_get_link_status() != PHY_LINK_ON) return false;
    if (!w5500_chip_init(&s_eth_cfg)) return false;
    Health_Heartbeat(HEALTH_ID_NET);
    start_services();
    return true;
}

/* -------------------------------------------------------------------------- */

static void NetTask_Function(void *arg) {
    (void)arg;
    INFO_PRINT("%s Task started\r\n", NET_TAG);

    /* 1) Wait for storage */
    if (!Storage_WaitReady(10000)) {
        WARNING_PRINT("%s Storage not ready, using defaults\r\n", NET_TAG);
    }
    Health_Heartbeat(HEALTH_ID_NET);

    /* 2-4) Hardware init */
    if (!apply_config_and_init()) {
        ERROR_PRINT("%s Fatal: Ethernet HW init failed. Safe loop.\r\n", NET_TAG);
        for (;;) {
            Health_Heartbeat(HEALTH_ID_NET);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }

    /* 5) Start services */
    s_last_link = w5500_get_link_status();
    start_services();
    Health_Heartbeat(HEALTH_ID_NET);

    uint32_t hb_ms = 0;

    /* 6) Main service loop */
    for (;;) {
        uint32_t now_ms = to_ms_since_boot(get_absolute_time());

        /* PHY link supervision */
        w5500_PhyLink cur_link = w5500_get_link_status();
        if (cur_link != s_last_link) {
            if (cur_link == PHY_LINK_ON) {
                INFO_PRINT("%s PHY link UP — reinitialising\r\n", NET_TAG);
                reinit_from_cache();
            } else {
                WARNING_PRINT("%s PHY link DOWN\r\n", NET_TAG);
            }
            s_last_link = cur_link;
        }

        /* Heartbeat */
        if ((now_ms - hb_ms) >= 250u) {
            hb_ms = now_ms;
            Health_Heartbeat(HEALTH_ID_NET);
        }

        /* HTTP service */
        http_server_process();

        vTaskDelay(pdMS_TO_TICKS(1));
        taskYIELD();

        /* SNMP service */
        SNMP_Tick10ms();
        SNMP_Poll(2);

        Health_Heartbeat(HEALTH_ID_NET);
        vTaskDelay(pdMS_TO_TICKS(NET_TASK_CYCLE_MS));
    }
}

/* -------------------------------------------------------------------------- */
/*  Public API                                                                */
/* -------------------------------------------------------------------------- */

BaseType_t NetTask_Init(bool enable) {
    if (!enable) return pdPASS;

    BaseType_t res = xTaskCreate(NetTask_Function, "Net", NET_STACK,
                                 NULL, NETTASK_PRIORITY, &s_net_task);
    if (res == pdPASS) {
        s_net_ready = true;
        INFO_PRINT("%s Task created\r\n", NET_TAG);
    }
    return res;
}

bool Net_IsReady(void) { return s_net_ready; }
