/**
 * @file web/status_handler.h
 * @brief GET /api/status handler — returns full system telemetry as JSON.
 *
 * JSON schema (matches index.html model):
 * {
 *   "sys":  { "ip": "x.x.x.x", "link": "100M Full", "uptime": <seconds> },
 *   "pd":   [ 8 × pdcard_entry ],
 *   "usbA": [ 4 × usba_entry ]
 * }
 *
 * @project PDNode-600 Pro
 */

#pragma once
#include "../CONFIG.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Serve the /api/status JSON response on the given socket. */
void handle_status_request(uint8_t sock);

#ifdef __cplusplus
}
#endif
