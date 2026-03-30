/**
 * @file tasks/ConsoleTask.c
 * @brief USB-CDC console command interface for PDNode-600 Pro.
 *
 * Polls stdio (USB-CDC) for character input, accumulates lines, and
 * dispatches complete commands to handler functions.
 *
 * @project PDNode-600 Pro
 * @version 1.0.0
 */

#include "../CONFIG.h"
#include <ctype.h>
#include "pico/bootrom.h"
#include "tusb.h"
#include "ConsoleTask.h"
#include "LoggerTask.h"
#include "HealthTask.h"
#include "StorageTask.h"
#include "PDCardTask.h"
#include "USBATask.h"
#include "../drivers/ethernet_driver.h"
#include "../drivers/socket.h"

#define CONSOLE_TAG     "[CONSOLE]"
#define LINE_BUF_SIZE   128
#define CONSOLE_POLL_MS 10

static TaskHandle_t s_console_task = NULL;
static volatile bool s_console_ready = false;

static char    s_line_buf[LINE_BUF_SIZE];
static int     s_line_len = 0;

/* -------------------------------------------------------------------------- */
/*  Helpers                                                                   */
/* -------------------------------------------------------------------------- */

static char *trim(char *str) {
    while (*str == ' ' || *str == '\t') str++;
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
        *end = '\0';
        end--;
    }
    return str;
}

static bool parse_ip(const char *str, uint8_t ip[4]) {
    int a, b, c, d;
    if (sscanf(str, "%d.%d.%d.%d", &a, &b, &c, &d) != 4) return false;
    if (a < 0 || a > 255 || b < 0 || b > 255 ||
        c < 0 || c > 255 || d < 0 || d > 255) return false;
    ip[0] = (uint8_t)a; ip[1] = (uint8_t)b;
    ip[2] = (uint8_t)c; ip[3] = (uint8_t)d;
    return true;
}

/* -------------------------------------------------------------------------- */
/*  Command handlers                                                          */
/* -------------------------------------------------------------------------- */

static void cmd_help(void) {
    ECHO("=== PDNode-600 Pro Console ===\r\n");
    ECHO("\r\n");
    ECHO("GENERAL\r\n");
    ECHO("  %-28s %s\r\n", "HELP | ?",         "Show this list");
    ECHO("  %-28s %s\r\n", "SYSINFO",          "System info: FW, HW, uptime, IP");
    ECHO("  %-28s %s\r\n", "REBOOT",           "Reboot the device");
    ECHO("  %-28s %s\r\n", "BOOTSEL",          "Enter USB ROM bootloader");
    ECHO("  %-28s %s\r\n", "RFS",              "Restore factory settings + reboot");
    ECHO("\r\n");
    ECHO("NETWORK  (reboot required to apply)\r\n");
    ECHO("  %-28s %s\r\n", "NETINFO",          "Show IP, mask, GW, DNS, MAC");
    ECHO("  %-28s %s\r\n", "SET_IP <a.b.c.d>", "Set static IP address");
    ECHO("  %-28s %s\r\n", "SET_SN <a.b.c.d>", "Set subnet mask");
    ECHO("  %-28s %s\r\n", "SET_GW <a.b.c.d>", "Set default gateway");
    ECHO("  %-28s %s\r\n", "SET_DNS <a.b.c.d>","Set DNS server");
    ECHO("  %-28s %s\r\n", "SET_DHCP <0|1>",   "0=static, 1=DHCP");
    ECHO("  %-28s %s\r\n", "SET_MAC <HH:HH:..>","Set MAC address");
    ECHO("\r\n");
    ECHO("IDENTITY\r\n");
    ECHO("  %-28s %s\r\n", "SET_NAME <name>",  "Set device name (max 31 chars)");
    ECHO("  %-28s %s\r\n", "SET_LOC <loc>",    "Set location string");
    ECHO("\r\n");
    ECHO("PD PORTS (USB-C PD)\r\n");
    ECHO("  %-28s %s\r\n", "PD STATUS",        "Show all 8 PD port status");
    ECHO("  %-28s %s\r\n", "PD STATUS <1-8>",  "Show single PD port status");
    ECHO("\r\n");
    ECHO("USB-A PORTS\r\n");
    ECHO("  %-28s %s\r\n", "USBA STATUS",       "Show all 4 USB-A port status");
    ECHO("  %-28s %s\r\n", "USBA STATUS <1-4>", "Show single USB-A port status");
    ECHO("  %-28s %s\r\n", "USBA ON <1-4>",     "Enable USB-A port");
    ECHO("  %-28s %s\r\n", "USBA OFF <1-4>",    "Disable USB-A port");
    ECHO("==============================\r\n");
}

static void cmd_sysinfo(void) {
    uint32_t uptime_s = to_ms_since_boot(get_absolute_time()) / 1000;
    uint32_t days    = uptime_s / 86400;
    uint32_t hours   = (uptime_s % 86400) / 3600;
    uint32_t minutes = (uptime_s % 3600) / 60;
    uint32_t seconds = uptime_s % 60;

    uint8_t ip[4] = {0};
    getSIPR(ip);
    w5500_PhyLink link = w5500_get_link_status();

    pdnode_identity_t id;
    Storage_GetIdentity(&id);

    pdnode_net_cfg_t net;
    Storage_GetNetConfig(&net);

    ECHO("========================================\r\n");
    ECHO("  PDNode-600 Pro\r\n");
    ECHO("  Firmware : %s\r\n", FIRMWARE_VERSION);
    ECHO("  Hardware : %s\r\n", HARDWARE_VERSION);
    ECHO("  Name     : %s\r\n", id.name);
    ECHO("  Location : %s\r\n", id.location);
    ECHO("  Uptime   : %lud %02lu:%02lu:%02lu\r\n",
         (unsigned long)days, (unsigned long)hours,
         (unsigned long)minutes, (unsigned long)seconds);
    ECHO("  IP       : %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
    ECHO("  Ethernet : %s\r\n", (link == PHY_LINK_ON) ? "100M Full" : "No Link");
    ECHO("  DHCP     : %s\r\n", net.dhcp ? "Enabled" : "Disabled");
    ECHO("========================================\r\n");
}

static void cmd_reboot(void) {
    ECHO("Rebooting...\r\n");
    vTaskDelay(pdMS_TO_TICKS(100));
    Health_RebootNow("CLI REBOOT");
}

static void cmd_bootsel(void) {
    ECHO("Entering BOOTSEL mode...\r\n");
    vTaskDelay(pdMS_TO_TICKS(50));
    taskENTER_CRITICAL();
    vTaskSuspendAll();
    reset_usb_boot(0, 0);
    for (;;) __asm volatile("wfi");
}

static void cmd_rfs(void) {
    ECHO("Restoring factory defaults...\r\n");
    pdnode_net_cfg_t net = {
        .ip   = DEFAULT_IP,
        .sn   = DEFAULT_SUBNET,
        .gw   = DEFAULT_GW,
        .dns  = DEFAULT_DNS,
        .mac  = DEFAULT_MAC,
        .dhcp = DEFAULT_DHCP,
    };
    pdnode_identity_t id;
    strncpy(id.name,     DEFAULT_DEVICE_NAME, sizeof(id.name)     - 1);
    strncpy(id.location, DEFAULT_LOCATION,    sizeof(id.location) - 1);
    id.name[sizeof(id.name) - 1] = '\0';
    id.location[sizeof(id.location) - 1] = '\0';
    Storage_SetNetConfig(&net);
    Storage_SetIdentity(&id);
    ECHO("Factory defaults written. Rebooting in 1 s...\r\n");
    vTaskDelay(pdMS_TO_TICKS(1000));
    Health_RebootNow("FACTORY RESET");
}

static void cmd_netinfo(void) {
    pdnode_net_cfg_t net;
    if (!Storage_GetNetConfig(&net)) {
        ECHO("ERROR: Could not read network config\r\n");
        return;
    }
    ECHO("Network Configuration:\r\n");
    ECHO("  MAC  : %02X:%02X:%02X:%02X:%02X:%02X\r\n",
         net.mac[0], net.mac[1], net.mac[2],
         net.mac[3], net.mac[4], net.mac[5]);
    ECHO("  IP   : %u.%u.%u.%u\r\n", net.ip[0],  net.ip[1],  net.ip[2],  net.ip[3]);
    ECHO("  Mask : %u.%u.%u.%u\r\n", net.sn[0],  net.sn[1],  net.sn[2],  net.sn[3]);
    ECHO("  GW   : %u.%u.%u.%u\r\n", net.gw[0],  net.gw[1],  net.gw[2],  net.gw[3]);
    ECHO("  DNS  : %u.%u.%u.%u\r\n", net.dns[0], net.dns[1], net.dns[2], net.dns[3]);
    ECHO("  DHCP : %s\r\n", net.dhcp ? "Enabled" : "Disabled");
}

static void cmd_set_ip(const char *args) {
    uint8_t ip[4];
    if (!parse_ip(args, ip)) { ECHO("ERROR: invalid IP\r\n"); return; }
    pdnode_net_cfg_t net;
    if (!Storage_GetNetConfig(&net)) { ECHO("ERROR: read failed\r\n"); return; }
    memcpy(net.ip, ip, 4);
    Storage_SetNetConfig(&net);
    ECHO("IP set to %u.%u.%u.%u (reboot required)\r\n", ip[0], ip[1], ip[2], ip[3]);
}

static void cmd_set_sn(const char *args) {
    uint8_t sn[4];
    if (!parse_ip(args, sn)) { ECHO("ERROR: invalid mask\r\n"); return; }
    pdnode_net_cfg_t net;
    if (!Storage_GetNetConfig(&net)) { ECHO("ERROR: read failed\r\n"); return; }
    memcpy(net.sn, sn, 4);
    Storage_SetNetConfig(&net);
    ECHO("Subnet set to %u.%u.%u.%u (reboot required)\r\n", sn[0], sn[1], sn[2], sn[3]);
}

static void cmd_set_gw(const char *args) {
    uint8_t gw[4];
    if (!parse_ip(args, gw)) { ECHO("ERROR: invalid gateway\r\n"); return; }
    pdnode_net_cfg_t net;
    if (!Storage_GetNetConfig(&net)) { ECHO("ERROR: read failed\r\n"); return; }
    memcpy(net.gw, gw, 4);
    Storage_SetNetConfig(&net);
    ECHO("Gateway set to %u.%u.%u.%u (reboot required)\r\n", gw[0], gw[1], gw[2], gw[3]);
}

static void cmd_set_dns(const char *args) {
    uint8_t dns[4];
    if (!parse_ip(args, dns)) { ECHO("ERROR: invalid DNS\r\n"); return; }
    pdnode_net_cfg_t net;
    if (!Storage_GetNetConfig(&net)) { ECHO("ERROR: read failed\r\n"); return; }
    memcpy(net.dns, dns, 4);
    Storage_SetNetConfig(&net);
    ECHO("DNS set to %u.%u.%u.%u (reboot required)\r\n", dns[0], dns[1], dns[2], dns[3]);
}

static void cmd_set_dhcp(const char *args) {
    if (!args || (*args != '0' && *args != '1')) {
        ECHO("Usage: SET_DHCP <0|1>\r\n"); return;
    }
    pdnode_net_cfg_t net;
    if (!Storage_GetNetConfig(&net)) { ECHO("ERROR: read failed\r\n"); return; }
    net.dhcp = (uint8_t)(*args - '0');
    Storage_SetNetConfig(&net);
    ECHO("DHCP %s (reboot required)\r\n", net.dhcp ? "enabled" : "disabled");
}

static void cmd_set_mac(const char *args) {
    unsigned int b[6];
    if (sscanf(args, "%x:%x:%x:%x:%x:%x",
               &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) != 6) {
        ECHO("Usage: SET_MAC AA:BB:CC:DD:EE:FF\r\n"); return;
    }
    pdnode_net_cfg_t net;
    if (!Storage_GetNetConfig(&net)) { ECHO("ERROR: read failed\r\n"); return; }
    for (int i = 0; i < 6; i++) net.mac[i] = (uint8_t)b[i];
    Storage_SetNetConfig(&net);
    ECHO("MAC set to %02X:%02X:%02X:%02X:%02X:%02X (reboot required)\r\n",
         net.mac[0], net.mac[1], net.mac[2],
         net.mac[3], net.mac[4], net.mac[5]);
}

static void cmd_set_name(const char *args) {
    if (!args || !*args) { ECHO("Usage: SET_NAME <name>\r\n"); return; }
    pdnode_identity_t id;
    Storage_GetIdentity(&id);
    strncpy(id.name, args, sizeof(id.name) - 1);
    id.name[sizeof(id.name) - 1] = '\0';
    Storage_SetIdentity(&id);
    ECHO("Device name set to: %s\r\n", id.name);
}

static void cmd_set_loc(const char *args) {
    if (!args || !*args) { ECHO("Usage: SET_LOC <location>\r\n"); return; }
    pdnode_identity_t id;
    Storage_GetIdentity(&id);
    strncpy(id.location, args, sizeof(id.location) - 1);
    id.location[sizeof(id.location) - 1] = '\0';
    Storage_SetIdentity(&id);
    ECHO("Location set to: %s\r\n", id.location);
}

static void print_pd_port(int p) {
    pdcard_telemetry_t t;
    bool ok = PDCard_GetTelemetry((uint8_t)p, &t);
    if (!ok) {
        memset(&t, 0, sizeof(t));
        t.connected = false;
        strncpy(t.port_state, "Disconnected", sizeof(t.port_state) - 1);
        strncpy(t.contract,   "None",         sizeof(t.contract)   - 1);
    }
    ECHO("  PD%d: %-12s  %s  %6.2fV  %5.3fA  %s\r\n",
         p + 1, t.port_state,
         t.pd_active ? "DFP" : "N/A",
         t.vbus_v, t.current_a,
         t.contract);
}

static void cmd_pd_status(const char *args) {
    if (!args || !*args) {
        ECHO("PD Port Status:\r\n");
        for (int i = 0; i < PDCARD_NUM_PORTS; i++) print_pd_port(i);
        return;
    }
    int n;
    if (sscanf(args, "%d", &n) != 1 || n < 1 || n > PDCARD_NUM_PORTS) {
        ECHO("Usage: PD STATUS [1-%d]\r\n", PDCARD_NUM_PORTS); return;
    }
    print_pd_port(n - 1);
}

static void print_usba_port(int p) {
    usba_telemetry_t t;
    bool ok = USBA_GetTelemetry((uint8_t)p, &t);
    if (!ok) memset(&t, 0, sizeof(t));
    ECHO("  USB-A%d: %-3s  fault=%-3s  %5.2fV  %5.3fA  %5.3fW\r\n",
         p + 1,
         t.enabled ? "ON" : "OFF",
         t.fault   ? "YES" : "no",
         t.voltage_v, t.current_a, t.power_w);
}

static void cmd_usba(const char *args) {
    if (!args || !*args) {
        ECHO("Usage: USBA STATUS [1-4] | ON <1-4> | OFF <1-4>\r\n"); return;
    }

    char sub[16] = {0};
    char rest[64] = {0};
    sscanf(args, "%15s %63[^\0]", sub, rest);
    for (char *p = sub; *p; p++) *p = (char)toupper((unsigned char)*p);

    if (strcmp(sub, "STATUS") == 0) {
        char *r = trim(rest);
        if (!*r) {
            ECHO("USB-A Port Status:\r\n");
            for (int i = 0; i < USBA_NUM_PORTS; i++) print_usba_port(i);
        } else {
            int n;
            if (sscanf(r, "%d", &n) != 1 || n < 1 || n > USBA_NUM_PORTS) {
                ECHO("Usage: USBA STATUS [1-%d]\r\n", USBA_NUM_PORTS); return;
            }
            print_usba_port(n - 1);
        }
        return;
    }

    bool turn_on = (strcmp(sub, "ON") == 0);
    bool turn_off = (strcmp(sub, "OFF") == 0);
    if (!turn_on && !turn_off) {
        ECHO("Usage: USBA STATUS | ON <1-4> | OFF <1-4>\r\n"); return;
    }

    int n;
    char *r = trim(rest);
    if (sscanf(r, "%d", &n) != 1 || n < 1 || n > USBA_NUM_PORTS) {
        ECHO("Usage: USBA %s <1-%d>\r\n", sub, USBA_NUM_PORTS); return;
    }
    if (USBA_SetEnable((uint8_t)(n - 1), turn_on)) {
        ECHO("USB-A%d %s\r\n", n, turn_on ? "enabled" : "disabled");
    } else {
        ECHO("ERROR: failed to set USB-A%d\r\n", n);
    }
}

/* -------------------------------------------------------------------------- */
/*  Command dispatcher                                                        */
/* -------------------------------------------------------------------------- */

static void dispatch_command(const char *line) {
    char buf[LINE_BUF_SIZE];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *trimmed = trim(buf);
    if (!*trimmed) return;

    /* Split "COMMAND args" */
    char *args = strchr(trimmed, ' ');
    if (args) {
        *args = '\0';
        args = trim(args + 1);
    }

    /* Uppercase command token */
    for (char *p = trimmed; *p; p++) *p = (char)toupper((unsigned char)*p);

    if (strcmp(trimmed, "HELP") == 0 || strcmp(trimmed, "?") == 0) {
        cmd_help();
    } else if (strcmp(trimmed, "SYSINFO") == 0) {
        cmd_sysinfo();
    } else if (strcmp(trimmed, "REBOOT") == 0) {
        cmd_reboot();
    } else if (strcmp(trimmed, "BOOTSEL") == 0) {
        cmd_bootsel();
    } else if (strcmp(trimmed, "RFS") == 0) {
        cmd_rfs();
    } else if (strcmp(trimmed, "NETINFO") == 0) {
        cmd_netinfo();
    } else if (strcmp(trimmed, "SET_IP") == 0) {
        cmd_set_ip(args ? args : "");
    } else if (strcmp(trimmed, "SET_SN") == 0) {
        cmd_set_sn(args ? args : "");
    } else if (strcmp(trimmed, "SET_GW") == 0) {
        cmd_set_gw(args ? args : "");
    } else if (strcmp(trimmed, "SET_DNS") == 0) {
        cmd_set_dns(args ? args : "");
    } else if (strcmp(trimmed, "SET_DHCP") == 0) {
        cmd_set_dhcp(args ? args : "");
    } else if (strcmp(trimmed, "SET_MAC") == 0) {
        cmd_set_mac(args ? args : "");
    } else if (strcmp(trimmed, "SET_NAME") == 0) {
        cmd_set_name(args ? args : "");
    } else if (strcmp(trimmed, "SET_LOC") == 0) {
        cmd_set_loc(args ? args : "");
    } else if (strcmp(trimmed, "PD") == 0) {
        /* "PD STATUS [n]" */
        char sub[16] = {0};
        char rest[32] = {0};
        if (args) sscanf(args, "%15s %31[^\0]", sub, rest);
        for (char *p = sub; *p; p++) *p = (char)toupper((unsigned char)*p);
        if (strcmp(sub, "STATUS") == 0) {
            cmd_pd_status(trim(rest));
        } else {
            ECHO("Usage: PD STATUS [1-%d]\r\n", PDCARD_NUM_PORTS);
        }
    } else if (strcmp(trimmed, "USBA") == 0) {
        cmd_usba(args ? args : "");
    } else {
        ECHO("Unknown command: '%s'. Type HELP for list.\r\n", trimmed);
    }
}

/* -------------------------------------------------------------------------- */
/*  Task function                                                             */
/* -------------------------------------------------------------------------- */

static void ConsoleTask_Function(void *arg) {
    (void)arg;

    /* Wait for LoggerTask to be ready before printing anything */
    {
        TickType_t t0 = xTaskGetTickCount();
        while (!Logger_IsReady() &&
               (xTaskGetTickCount() - t0) < pdMS_TO_TICKS(5000)) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    ECHO("\r\n");
    ECHO("=== PDNode-600 Pro Console Ready ===\r\n");
    ECHO("Type HELP for available commands\r\n");
    ECHO("\r\n");

    static uint32_t hb_ms = 0;

    for (;;) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if ((now - hb_ms) >= 500u) {
            hb_ms = now;
            Health_Heartbeat(HEALTH_ID_CONSOLE);
        }

        if (tud_cdc_available()) {
            uint8_t buf[64];
            uint32_t count = tud_cdc_read(buf, sizeof(buf));
            for (uint32_t i = 0u; i < count; i++) {
                char ch = (char)buf[i];
                if (ch == '\b' || ch == 0x7F) {
                    if (s_line_len > 0) s_line_len--;
                } else if (ch == '\r' || ch == '\n') {
                    s_line_buf[s_line_len] = '\0';
                    if (s_line_len > 0) {
                        dispatch_command(s_line_buf);
                        s_line_len = 0;
                    }
                } else if (s_line_len < (int)(sizeof(s_line_buf) - 1)) {
                    s_line_buf[s_line_len++] = ch;
                }
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(CONSOLE_POLL_MS));
        }
    }
}

/* -------------------------------------------------------------------------- */
/*  Public API                                                                */
/* -------------------------------------------------------------------------- */

BaseType_t ConsoleTask_Init(bool enable) {
    if (!enable) return pdPASS;

    BaseType_t res = xTaskCreate(ConsoleTask_Function, "Console",
                                 CONSOLE_STACK_SIZE, NULL,
                                 CONSOLETASK_PRIORITY, &s_console_task);
    if (res == pdPASS) {
        s_console_ready = true;
    }
    return res;
}

bool Console_IsReady(void) { return s_console_ready; }
