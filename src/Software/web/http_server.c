/**
 * @file web/http_server.c
 * @brief HTTP/1.1 server for PDNode-600 Pro web dashboard.
 *
 * Routes:
 *   GET /            → index.html (embedded, gzip)
 *   GET /index.html  → index.html (embedded, gzip)
 *   GET /api/status  → JSON status payload
 *   GET /api/info    → JSON device info
 *   POST /api/usba   → Enable/disable USB-A port
 *
 * @project PDNode-600 Pro
 * @version 1.0.0
 */

#include "../CONFIG.h"
#include "http_server.h"
#include "status_handler.h"
#include "index_html.h"
#include "../tasks/HealthTask.h"
#include "../tasks/USBATask.h"
#include "../drivers/socket.h"
#include "../drivers/ethernet_driver.h"

#define HTTP_TAG        "[HTTP]"
#define HTTP_PORT       80
#define HTTP_SOCK       0
#define HTTP_BUF_SIZE   2048
#define TX_CHUNK        4096

static char    *s_buf     = NULL;
static int8_t   s_sock    = HTTP_SOCK;

/* -------------------------------------------------------------------------- */
/*  Low-level helpers                                                         */
/* -------------------------------------------------------------------------- */

static int send_all(uint8_t sock, const uint8_t *data, int len) {
    int sent = 0;
    while (sent < len) {
        int chunk = (len - sent > TX_CHUNK) ? TX_CHUNK : (len - sent);
        int n = send(sock, (uint8_t *)(data + sent), (uint16_t)chunk);
        if (n <= 0) return -1;
        sent += n;
        Health_Heartbeat(HEALTH_ID_NET);
        if (sent < len) vTaskDelay(pdMS_TO_TICKS(5));
    }
    return sent;
}

static void send_response(uint8_t sock, int status_code,
                          const char *content_type,
                          const uint8_t *body, int body_len,
                          bool gzip) {
    char hdr[320];
    const char *status_str = (status_code == 200) ? "OK"
                           : (status_code == 404) ? "Not Found"
                           : "Bad Request";
    int hlen;
    if (gzip) {
        hlen = snprintf(hdr, sizeof(hdr),
            "HTTP/1.1 %d %s\r\n"
            "Content-Type: %s\r\n"
            "Content-Encoding: gzip\r\n"
            "Content-Length: %d\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Cache-Control: no-cache\r\n"
            "Connection: close\r\n\r\n",
            status_code, status_str, content_type, body_len);
    } else {
        hlen = snprintf(hdr, sizeof(hdr),
            "HTTP/1.1 %d %s\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %d\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Cache-Control: no-cache\r\n"
            "Connection: close\r\n\r\n",
            status_code, status_str, content_type, body_len);
    }
    if (hlen < 0 || hlen >= (int)sizeof(hdr)) hlen = (int)sizeof(hdr) - 1;
    send_all(sock, (const uint8_t *)hdr, hlen);
    if (body && body_len > 0) {
        send_all(sock, body, body_len);
    }
}

static int read_request(uint8_t sock, char *buf, int buflen) {
    int total = 0;
    while (total < buflen - 1) {
        int n = recv(sock, (uint8_t *)buf + total, buflen - 1 - total);
        if (n <= 0) break;
        total += n;
        buf[total] = '\0';
        if (strstr(buf, "\r\n\r\n")) break;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    return total;
}

/* -------------------------------------------------------------------------- */
/*  Route handlers                                                            */
/* -------------------------------------------------------------------------- */

static void handle_index(uint8_t sock) {
    send_response(sock, 200, "text/html",
                  (const uint8_t *)INDEX_HTML_GZ, INDEX_HTML_GZ_LEN, true);
}

static void handle_usba_post(uint8_t sock, const char *req_buf) {
    /* Find body after \r\n\r\n */
    const char *body = strstr(req_buf, "\r\n\r\n");
    if (!body) {
        const char *err = "{\"ok\":false,\"error\":\"no body\"}";
        send_response(sock, 400, "application/json",
                      (const uint8_t *)err, (int)strlen(err), false);
        return;
    }
    body += 4;

    /* Parse "port" and "enabled" from JSON body — simple search, no lib needed */
    int port    = -1;
    int enabled = -1;

    const char *p = strstr(body, "\"port\":");
    if (p) port = (int)strtol(p + 7, NULL, 10); /* p+7 skips "port": to digit */

    const char *e = strstr(body, "\"enabled\":");
    if (e) {
        e += 10;
        while (*e == ' ') e++;
        enabled = (strncmp(e, "true", 4) == 0) ? 1 : 0;
    }

    char resp[64];
    int rlen;
    if (port < 1 || port > USBA_NUM_PORTS || enabled < 0) {
        rlen = snprintf(resp, sizeof(resp),
                        "{\"ok\":false,\"error\":\"bad params\",\"port\":%d}", port);
        send_response(sock, 400, "application/json",
                      (const uint8_t *)resp, rlen, false);
        return;
    }

    bool ok = USBA_SetEnable((uint8_t)(port - 1), enabled == 1);
    rlen = snprintf(resp, sizeof(resp),
                    "{\"ok\":%s,\"port\":%d,\"enabled\":%s}",
                    ok ? "true" : "false", port, enabled ? "true" : "false");
    send_response(sock, 200, "application/json",
                  (const uint8_t *)resp, rlen, false);
}

static void handle_not_found(uint8_t sock) {
    const char *body = "<h1>404 Not Found</h1>";
    send_response(sock, 404, "text/html",
                  (const uint8_t *)body, (int)strlen(body), false);
}

static void handle_api_info(uint8_t sock) {
    /* Build device info JSON */
    char json[256];

    /* Read device IP from W5500 */
    uint8_t ip[4];
    getSIPR(ip);

    int len = snprintf(json, sizeof(json),
        "{\"fw\":\"%s\",\"hw\":\"%s\","
        "\"ip\":\"%u.%u.%u.%u\","
        "\"name\":\"PDNode-600 Pro\","
        "\"ports_pd\":8,\"ports_usba\":4}",
        FIRMWARE_VERSION, HARDWARE_VERSION,
        ip[0], ip[1], ip[2], ip[3]);

    send_response(sock, 200, "application/json",
                  (const uint8_t *)json, len, false);
}

/* -------------------------------------------------------------------------- */
/*  Public API                                                                */
/* -------------------------------------------------------------------------- */

bool http_server_init(void) {
    if (!s_buf) {
        s_buf = pvPortMalloc(HTTP_BUF_SIZE);
        if (!s_buf) {
            ERROR_PRINT("%s Buffer alloc failed\r\n", HTTP_TAG);
            return false;
        }
    }

    s_sock = socket(HTTP_SOCK, Sn_MR_TCP, HTTP_PORT, 0);
    if (s_sock < 0) {
        ERROR_PRINT("%s Socket open failed\r\n", HTTP_TAG);
        return false;
    }

    if (listen(s_sock) != SOCK_OK) {
        ERROR_PRINT("%s listen() failed\r\n", HTTP_TAG);
        return false;
    }

    INFO_PRINT("%s Listening on port %d\r\n", HTTP_TAG, HTTP_PORT);
    return true;
}

void http_server_process(void) {
    switch (getSn_SR(s_sock)) {

    case SOCK_ESTABLISHED: {
        if (getSn_IR(s_sock) & Sn_IR_CON)
            setSn_IR(s_sock, Sn_IR_CON);

        int n = read_request((uint8_t)s_sock, s_buf, HTTP_BUF_SIZE);
        if (n <= 0) break;
        s_buf[n] = '\0';

        /* Route matching */
        if (!strncmp(s_buf, "GET /api/status", 15)) {
            handle_status_request((uint8_t)s_sock);
        } else if (!strncmp(s_buf, "GET /api/info", 13)) {
            handle_api_info((uint8_t)s_sock);
        } else if (!strncmp(s_buf, "POST /api/usba", 14)) {
            handle_usba_post((uint8_t)s_sock, s_buf);
        } else if (!strncmp(s_buf, "GET / ", 6) ||
                   !strncmp(s_buf, "GET /index.html", 15)) {
            handle_index((uint8_t)s_sock);
        } else {
            handle_not_found((uint8_t)s_sock);
        }

        disconnect(s_sock);
        closesocket(s_sock);
        s_sock = socket(HTTP_SOCK, Sn_MR_TCP, HTTP_PORT, 0);
        if (s_sock >= 0) listen(s_sock);
        break;
    }

    case SOCK_CLOSE_WAIT:
        disconnect(s_sock);
        /* fall through */
    case SOCK_CLOSED:
        closesocket(s_sock);
        s_sock = socket(HTTP_SOCK, Sn_MR_TCP, HTTP_PORT, 0);
        if (s_sock >= 0) listen(s_sock);
        break;

    case SOCK_INIT:
        listen(s_sock);
        break;

    default:
        break;
    }
}
