/**
 * @file tasks/NetTask.h
 * @brief W5500 Ethernet task — brings up network stack, HTTP server, SNMP agent.
 *
 * NetTask waits for StorageTask to be ready, reads network config,
 * configures W5500, then runs the HTTP + SNMP service loop.
 *
 * @project PDNode-600 Pro
 */

#pragma once
#include "../CONFIG.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Create and start NetTask. Must be called after StorageTask_Init(). */
BaseType_t NetTask_Init(bool enable);

/** Returns true once the task has been successfully created. */
bool Net_IsReady(void);

#ifdef __cplusplus
}
#endif
