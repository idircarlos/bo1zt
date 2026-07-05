#include <winsock2.h>
#include <ws2tcpip.h>

#include "win/http.h"
#include "logger.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define REQ_BUFFER_SIZE 8192
#define CLIENT_BUFFER_SIZE 65536

struct HttpResponse {
    int status;
    char contentType[64];
    char *body;
    bool bodySet;
};

void httpResponseSet(HttpResponse *response, int status, const char *contentType, const char *body) {
    if (!response) return;
    response->status = status;
    snprintf(response->contentType, sizeof(response->contentType), "%s",
             contentType ? contentType : "application/json");
    free(response->body);
    response->body = body ? _strdup(body) : NULL;
    response->bodySet = true;
}

void httpResponseJson(HttpResponse *response, int status, const char *jsonBody) {
    httpResponseSet(response, status, "application/json", jsonBody);
}

void httpResponseStatus(HttpResponse *response, int status) {
    httpResponseSet(response, status, "application/json", NULL);
}

static const char *reasonPhrase(int status) {
    switch (status) {
        case 200: return "OK";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 409: return "Conflict";
        case 500: return "Internal Server Error";
        default:  return "OK";
    }
}

static void sendResponse(SOCKET client, HttpResponse *response) {
    char header[512];
    int bodyLen = response->body ? (int)strlen(response->body) : 0;
    int headerLen = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n",
        response->status, reasonPhrase(response->status),
        response->contentType, bodyLen);
    if (headerLen > 0) send(client, header, headerLen, 0);
    if (bodyLen > 0) send(client, response->body, bodyLen, 0);
}

static int readRequest(SOCKET client, char *buf, int bufSize) {
    int total = 0;
    int headerEnd = -1;
    int contentLength = 0;
    while (total < bufSize - 1) {
        int n = recv(client, buf + total, bufSize - 1 - total, 0);
        if (n <= 0) break;
        total += n;
        buf[total] = '\0';
        if (headerEnd < 0) {
            char *he = strstr(buf, "\r\n\r\n");
            if (he) {
                headerEnd = (int)(he - buf) + 4;
                const char *cl = strstr(buf, "Content-Length:");
                if (!cl) cl = strstr(buf, "content-length:");
                if (cl) contentLength = atoi(cl + 15);
            }
        }
        if (headerEnd >= 0 && total >= headerEnd + contentLength) break;
    }
    return total;
}

static void handleConnection(SOCKET client, HttpHandler handler, void *userData) {
    char buf[REQ_BUFFER_SIZE];
    int total = readRequest(client, buf, sizeof(buf));

    HttpResponse response;
    memset(&response, 0, sizeof(response));
    response.status = 200;
    snprintf(response.contentType, sizeof(response.contentType), "application/json");

    if (total <= 0) return;

    char method[8] = {0};
    char rawPath[1024] = {0};
    if (sscanf(buf, "%7s %1023s", method, rawPath) != 2) {
        httpResponseJson(&response, 400,
            "{\"error\":{\"code\":\"INVALID_PARAM\",\"message\":\"Malformed request line\"}}");
        sendResponse(client, &response);
        free(response.body);
        return;
    }

    char *query = strchr(rawPath, '?');
    if (query) { *query = '\0'; query++; } else { query = (char *)""; }

    const char *body = strstr(buf, "\r\n\r\n");
    body = body ? body + 4 : "";

    HttpRequest request = { method, rawPath, query, body };
    handler(&request, &response, userData);

    if (!response.bodySet) {
        httpResponseJson(&response, 404,
            "{\"error\":{\"code\":\"NOT_FOUND\",\"message\":\"Unknown resource\"}}");
    }
    sendResponse(client, &response);
    free(response.body);
}

int httpServe(int port, HttpHandler handler, void *userData) {
    if (!handler) return 1;

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        LOG_ERROR("HTTP: WSAStartup failed");
        return 1;
    }

    SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET) {
        LOG_ERROR("HTTP: socket() failed");
        WSACleanup();
        return 1;
    }

    int reuse = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        LOG_ERROR("HTTP: bind() failed on 127.0.0.1:%d", port);
        closesocket(listener);
        WSACleanup();
        return 1;
    }

    if (listen(listener, SOMAXCONN) == SOCKET_ERROR) {
        LOG_ERROR("HTTP: listen() failed");
        closesocket(listener);
        WSACleanup();
        return 1;
    }

    LOG_INFO("HTTP listening on http://127.0.0.1:%d", port);

    while (true) {
        SOCKET client = accept(listener, NULL, NULL);
        if (client == INVALID_SOCKET) continue;
        handleConnection(client, handler, userData);
        closesocket(client);
    }

    closesocket(listener);
    WSACleanup();
    return 0;
}

HttpClientResponse httpClientRequest(int port, const char *method, const char *path, const char *body) {
    HttpClientResponse result = { -1, NULL };

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return result;

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) { WSACleanup(); return result; }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(sock);
        WSACleanup();
        return result;
    }

    int bodyLen = body ? (int)strlen(body) : 0;
    int headerLen = snprintf(NULL, 0,
        "%s %s HTTP/1.1\r\nHost: 127.0.0.1:%d\r\nContent-Type: application/json\r\n"
        "Content-Length: %d\r\nConnection: close\r\n\r\n",
        method, path, port, bodyLen);
    char *request = (char *)malloc(headerLen + bodyLen + 1);
    if (!request) { closesocket(sock); WSACleanup(); return result; }
    int reqLen = snprintf(request, headerLen + 1,
        "%s %s HTTP/1.1\r\nHost: 127.0.0.1:%d\r\nContent-Type: application/json\r\n"
        "Content-Length: %d\r\nConnection: close\r\n\r\n",
        method, path, port, bodyLen);
    if (bodyLen > 0) memcpy(request + reqLen, body, bodyLen);
    int totalReq = reqLen + bodyLen;

    if (send(sock, request, totalReq, 0) == SOCKET_ERROR) {
        free(request);
        closesocket(sock);
        WSACleanup();
        return result;
    }
    free(request);

    char *resp = (char *)malloc(CLIENT_BUFFER_SIZE);
    if (!resp) { closesocket(sock); WSACleanup(); return result; }
    int total = 0;
    int n;
    while (total < CLIENT_BUFFER_SIZE - 1 &&
           (n = recv(sock, resp + total, CLIENT_BUFFER_SIZE - 1 - total, 0)) > 0) {
        total += n;
    }
    resp[total] = '\0';

    closesocket(sock);
    WSACleanup();

    sscanf(resp, "HTTP/1.1 %d", &result.status);
    const char *bodyStart = strstr(resp, "\r\n\r\n");
    if (bodyStart) {
        result.body = _strdup(bodyStart + 4);
    }
    free(resp);
    return result;
}

void httpClientResponseFree(HttpClientResponse *response) {
    if (!response) return;
    free(response->body);
    response->body = NULL;
}
