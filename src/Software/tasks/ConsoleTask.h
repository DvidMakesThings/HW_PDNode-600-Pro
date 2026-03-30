/**
 * @file tasks/ConsoleTask.h
 * @brief USB-CDC console command interface for PDNode-600 Pro.
 *
 * Polls stdio (USB-CDC) for command input and dispatches to handlers.
 * Provides system info, network configuration, port control, and diagnostics.
 *
 * Commands:
 *   HELP          — List all commands
 *   SYSINFO       — Firmware, hardware, uptime, network
 *   REBOOT        — Watchdog reboot
 *   BOOTSEL       — Enter USB ROM bootloader
 *   RFS           — Restore factory defaults and reboot
 *   NETINFO       — Display current network config
 *   SET_IP  <ip>  — Set static IP (reboot required)
 *   SET_SN  <sn>  — Set subnet mask
 *   SET_GW  <gw>  — Set gateway
 *   SET_DNS <dns> — Set DNS server
 *   SET_DHCP <0|1>— Disable/enable DHCP
 *   SET_MAC <mac> — Set MAC address (AA:BB:CC:DD:EE:FF)
 *   SET_NAME <n>  — Set device name (max 31 chars)
 *   SET_LOC  <l>  — Set location string
 *   PD STATUS     — Show all 8 PD port telemetry
 *   PD STATUS <n> — Show single PD port (1-8)
 *   USBA STATUS   — Show all 4 USB-A port telemetry
 *   USBA STATUS <n>— Show single USB-A port (1-4)
 *   USBA ON  <n>  — Enable USB-A port (1-4)
 *   USBA OFF <n>  — Disable USB-A port (1-4)
 *
 * @project PDNode-600 Pro
 * @version 1.0.0
 */

#pragma once
#include "../CONFIG.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Create and start the console task. */
BaseType_t ConsoleTask_Init(bool enable);

/** Returns true after ConsoleTask_Init(true) succeeds. */
bool Console_IsReady(void);

#ifdef __cplusplus
}
#endif
