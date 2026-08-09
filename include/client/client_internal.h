#ifndef CLIENT_INTERNAL_H_
#define CLIENT_INTERNAL_H_

#include "client.h"
#include "utils/json.h"
#include "utils/color.h"
#include "utils/common.h" // Rect

#define CLIENT_API_BASE "/api/v1"
#define CLIENT_ERROR_MAX 128

struct Client {
    int port;
    char errorCode[CLIENT_ERROR_MAX];
    char errorMessage[CLIENT_ERROR_MAX];
};

ClientResult clientRequest(Client *client, const char *method, const char *path,
                           const char *body, JsonValue **out);

void clientClearError(Client *client);

JsonValue *clientColorJson(RGBAColor color);
JsonValue *clientRectJson(Rect rect);
bool clientParseColor(const JsonValue *obj, RGBAColor *out);
bool clientParseRect(const JsonValue *obj, Rect *out);

#endif // CLIENT_INTERNAL_H_
