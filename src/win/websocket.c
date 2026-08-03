#include <windows.h>
#include <winhttp.h>

#include "win/websocket.h"
#include "win/text.h"
#include "logger.h"

#include <stdlib.h>
#include <string.h>

#define RESOLVE_TIMEOUT 10000
#define CONNECT_TIMEOUT 15000
#define SEND_TIMEOUT    15000
#define KEEPALIVE_INTERVAL 30000

struct WebSocket {
    HINTERNET session;
    HINTERNET connection;
    HINTERNET socket;
};

WebSocket *webSocketConnect(const char *host, int port, const char *path, int receiveTimeoutMs) {
    if (!host || !path) return NULL;

    WebSocket *socket = (WebSocket *)calloc(1, sizeof(WebSocket));
    if (!socket) return NULL;

    wchar_t *wideHost = textToWide(host);
    wchar_t *widePath = textToWide(path);
    HINTERNET request = NULL;
    DWORD keepAlive = KEEPALIVE_INTERVAL;

    if (!wideHost || !widePath) { LOG_ERROR("WebSocket: out of memory"); goto fail; }

    socket->session = WinHttpOpen(L"bo1zt/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                  WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!socket->session) { LOG_ERROR("WebSocket: WinHttpOpen failed (%lu)", GetLastError()); goto fail; }

    WinHttpSetTimeouts(socket->session, RESOLVE_TIMEOUT, CONNECT_TIMEOUT,
                       SEND_TIMEOUT, receiveTimeoutMs);

    socket->connection = WinHttpConnect(socket->session, wideHost, (INTERNET_PORT)port, 0);
    if (!socket->connection) { LOG_ERROR("WebSocket: WinHttpConnect failed (%lu)", GetLastError()); goto fail; }

    request = WinHttpOpenRequest(socket->connection, L"GET", widePath, NULL,
                                 WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                 WINHTTP_FLAG_SECURE);
    if (!request) { LOG_ERROR("WebSocket: WinHttpOpenRequest failed (%lu)", GetLastError()); goto fail; }

    if (!WinHttpSetOption(request, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, NULL, 0)) {
        LOG_ERROR("WebSocket: upgrade option rejected (%lu)", GetLastError());
        goto fail;
    }

    if (!WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        LOG_ERROR("WebSocket: WinHttpSendRequest failed (%lu)", GetLastError());
        goto fail;
    }

    if (!WinHttpReceiveResponse(request, NULL)) {
        LOG_ERROR("WebSocket: WinHttpReceiveResponse failed (%lu)", GetLastError());
        goto fail;
    }

    socket->socket = WinHttpWebSocketCompleteUpgrade(request, 0);
    if (!socket->socket) { LOG_ERROR("WebSocket: upgrade failed (%lu)", GetLastError()); goto fail; }

    WinHttpSetOption(socket->socket, WINHTTP_OPTION_WEB_SOCKET_KEEPALIVE_INTERVAL,
                     &keepAlive, sizeof(keepAlive));

    WinHttpCloseHandle(request);
    free(wideHost);
    free(widePath);
    return socket;

fail:
    if (request) WinHttpCloseHandle(request);
    free(wideHost);
    free(widePath);
    webSocketClose(socket);
    return NULL;
}

void webSocketShutdown(WebSocket *socket) {
    if (!socket || !socket->socket) return;
    WinHttpWebSocketShutdown(socket->socket, WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, NULL, 0);
}

void webSocketClose(WebSocket *socket) {
    if (!socket) return;
    if (socket->socket) {
        WinHttpWebSocketClose(socket->socket, WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, NULL, 0);
        WinHttpCloseHandle(socket->socket);
    }
    if (socket->connection) WinHttpCloseHandle(socket->connection);
    if (socket->session) WinHttpCloseHandle(socket->session);
    free(socket);
}

static bool isFinalFragment(WINHTTP_WEB_SOCKET_BUFFER_TYPE type) {
    return type == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE ||
           type == WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE;
}

WebSocketResult webSocketReceiveText(WebSocket *socket, char *out, int size, int *length) {
    if (length) *length = 0;
    if (!socket || !out || size < 2) return WEBSOCKET_ERROR;

    int total = 0;
    for (;;) {
        DWORD read = 0;
        WINHTTP_WEB_SOCKET_BUFFER_TYPE type = WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE;
        DWORD error = WinHttpWebSocketReceive(socket->socket, out + total,
                                              (DWORD)(size - 1 - total), &read, &type);
        if (error == ERROR_WINHTTP_TIMEOUT) return WEBSOCKET_TIMEOUT;
        if (error != ERROR_SUCCESS) {
            LOG_ERROR("WebSocket: receive failed (%lu)", error);
            return WEBSOCKET_ERROR;
        }
        if (type == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE) return WEBSOCKET_CLOSED;

        total += (int)read;
        if (isFinalFragment(type)) break;

        if (total >= size - 1) {
            LOG_ERROR("WebSocket: message does not fit in %d bytes", size - 1);
            return WEBSOCKET_ERROR;
        }
    }

    out[total] = '\0';
    if (length) *length = total;
    return WEBSOCKET_MESSAGE;
}
