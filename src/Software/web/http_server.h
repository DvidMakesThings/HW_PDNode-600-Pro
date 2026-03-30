/**
 * @file web/http_server.h
 * @brief Minimal HTTP/1.1 server on W5500 socket 0 (port 80).
 *
 * @project PDNode-600 Pro
 */

#pragma once
#include "../CONFIG.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Open the TCP socket and start listening on port 80. */
bool http_server_init(void);

/**
 * @brief Process one HTTP connection cycle.
 * Call repeatedly from NetTask's main loop.
 */
void http_server_process(void);

#ifdef __cplusplus
}
#endif
