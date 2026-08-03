#ifndef WIN_WEBSOCKET_H_
#define WIN_WEBSOCKET_H_

#include <stdbool.h>

typedef struct WebSocket WebSocket;

typedef enum {
    WEBSOCKET_MESSAGE,
    WEBSOCKET_TIMEOUT,
    WEBSOCKET_CLOSED,
    WEBSOCKET_ERROR,
} WebSocketResult;

WebSocket *webSocketConnect(const char *host, int port, const char *path, int receiveTimeoutMs);
void webSocketShutdown(WebSocket *socket);
void webSocketClose(WebSocket *socket);
WebSocketResult webSocketReceiveText(WebSocket *socket, char *out, int size, int *length);

#endif // WIN_WEBSOCKET_H_
