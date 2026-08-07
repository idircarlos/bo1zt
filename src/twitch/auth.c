#include "twitch/auth.h"
#include "twitch/twitch_internal.h"
#include "win/dpapi.h"
#include "win/file.h"
#include "utils/json.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define FORM_HEADERS "Content-Type: application/x-www-form-urlencoded\r\n"
#define DEVICE_GRANT "urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Adevice_code"

#define TOKEN_FOLDER "bo1zt\\twitch"
#define TOKEN_FILE   "tokens.dat"

#define TOKEN_BLOB_SIZE 1280
#define TOKEN_PATH_SIZE 512

static void formEncode(const char *in, char *out, size_t size) {
    static const char HEX[] = "0123456789ABCDEF";
    size_t written = 0;
    for (; *in && written + 4 < size; ++in) {
        unsigned char c = (unsigned char)*in;
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out[written++] = (char)c;
        } else if (c == ' ') {
            out[written++] = '+';
        } else {
            out[written++] = '%';
            out[written++] = HEX[c >> 4];
            out[written++] = HEX[c & 0x0F];
        }
    }
    out[written] = '\0';
}

static TwitchResult storeTokenPair(TwitchClient *client, const JsonValue *response) {
    snprintf(client->token, sizeof(client->token), "%s",
             jsonObjectGetString(response, "access_token", ""));
    snprintf(client->refreshToken, sizeof(client->refreshToken), "%s",
             jsonObjectGetString(response, "refresh_token", ""));

    if (!client->token[0] || !client->refreshToken[0]) {
        twitchSetError(client, "the token response carried no token pair");
        return TWITCH_ERR_PROTOCOL;
    }

    // The pair may belong to another user, so the identity is unknown until revalidated.
    client->login[0] = '\0';
    client->userId[0] = '\0';

    if (client->listener.handler) client->listener.handler(client, client->listener.context);
    return TWITCH_OK;
}

static TwitchResult requestTokenPair(TwitchClient *client, const char *body) {
    JsonValue *response = NULL;
    TwitchResult r = twitchRequest(client, TWITCH_ID_HOST, "POST", "/oauth2/token",
                                   FORM_HEADERS, body, &response);
    if (r != TWITCH_OK) return r;
    if (!response) return TWITCH_ERR_PROTOCOL;

    r = storeTokenPair(client, response);
    jsonFree(response);
    return r;
}

TwitchResult twitchAuthStart(TwitchClient *client, TwitchAuthFlow *out) {
    if (!client || !out) return TWITCH_ERR_INVALID_PARAM;
    memset(out, 0, sizeof(*out));

    char scopes[256];
    formEncode(TWITCH_SCOPES, scopes, sizeof(scopes));

    char body[512];
    snprintf(body, sizeof(body), "client_id=%s&scopes=%s", client->clientId, scopes);

    JsonValue *response = NULL;
    TwitchResult r = twitchRequest(client, TWITCH_ID_HOST, "POST", "/oauth2/device",
                                   FORM_HEADERS, body, &response);
    if (r != TWITCH_OK) return r;
    if (!response) return TWITCH_ERR_PROTOCOL;

    snprintf(out->deviceCode, sizeof(out->deviceCode), "%s",
             jsonObjectGetString(response, "device_code", ""));
    snprintf(out->userCode, sizeof(out->userCode), "%s",
             jsonObjectGetString(response, "user_code", ""));
    snprintf(out->verificationUri, sizeof(out->verificationUri), "%s",
             jsonObjectGetString(response, "verification_uri", ""));
    out->interval = jsonObjectGetInt(response, "interval", 5);
    out->expiresIn = jsonObjectGetInt(response, "expires_in", 1800);
    jsonFree(response);

    if (!out->deviceCode[0] || !out->verificationUri[0]) {
        twitchSetError(client, "the device response carried no code");
        return TWITCH_ERR_PROTOCOL;
    }
    return TWITCH_OK;
}

// The device endpoint reports "the user has not clicked authorize yet" as a 400 with this message.
static bool userHasNotAuthorizedYet(const TwitchClient *client) {
    return strcmp(twitchLastError(client), "authorization_pending") == 0;
}

TwitchResult twitchAuthPoll(TwitchClient *client, const TwitchAuthFlow *flow) {
    if (!client || !flow) return TWITCH_ERR_INVALID_PARAM;

    char scopes[256];
    formEncode(TWITCH_SCOPES, scopes, sizeof(scopes));

    char body[768];
    snprintf(body, sizeof(body), "client_id=%s&scopes=%s&device_code=%s&grant_type=%s",
             client->clientId, scopes, flow->deviceCode, DEVICE_GRANT);

    TwitchResult r = requestTokenPair(client, body);
    if (r == TWITCH_ERR_HTTP && userHasNotAuthorizedYet(client)) return TWITCH_PENDING;
    return r;
}

TwitchResult twitchAuthRefresh(TwitchClient *client) {
    if (!client) return TWITCH_ERR_INVALID_PARAM;
    if (!client->refreshToken[0]) {
        twitchSetError(client, "no refresh token stored");
        return TWITCH_ERR_AUTH;
    }

    char refresh[768];
    formEncode(client->refreshToken, refresh, sizeof(refresh));

    char body[1024];
    snprintf(body, sizeof(body), "client_id=%s&grant_type=refresh_token&refresh_token=%s",
             client->clientId, refresh);

    TwitchResult r = requestTokenPair(client, body);
    if (r != TWITCH_OK) LOG_ERROR("Twitch: refresh failed: %s", twitchLastError(client));
    return r;
}

static bool tokenFolder(char *out, size_t size) {
    if (!fileAppDataPath(out, size, TOKEN_FOLDER)) {
        LOG_ERROR("Twitch: failed to resolve %%APPDATA%%");
        return false;
    }
    return true;
}

static bool tokenPath(char *out, size_t size) {
    char folder[TOKEN_PATH_SIZE];
    if (!tokenFolder(folder, sizeof(folder))) return false;

    int length = snprintf(out, size, "%s\\%s", folder, TOKEN_FILE);
    return length > 0 && (size_t)length < size;
}

bool twitchAuthSave(const TwitchClient *client) {
    if (!client) return false;

    char folder[TOKEN_PATH_SIZE];
    char path[TOKEN_PATH_SIZE];
    if (!tokenFolder(folder, sizeof(folder)) || !tokenPath(path, sizeof(path))) return false;
    if (!fileCreateFolder(folder)) {
        LOG_ERROR("Twitch: could not create %s", folder);
        return false;
    }

    char blob[TOKEN_BLOB_SIZE];
    int length = snprintf(blob, sizeof(blob), "%s\n%s\n%s", client->clientId, client->token,
                          client->refreshToken);
    if (length < 0 || length >= (int)sizeof(blob)) return false;

    void *encrypted = NULL;
    size_t encryptedSize = 0;
    if (!dpapiProtect(blob, (size_t)length, &encrypted, &encryptedSize)) return false;

    bool saved = fileWriteAll(path, encrypted, encryptedSize);
    dpapiFree(encrypted);
    if (!saved) LOG_ERROR("Twitch: could not write %s", path);
    return saved;
}

static char *decryptBlob(const char *path) {
    size_t encryptedSize = 0;
    void *encrypted = fileReadAll(path, &encryptedSize);
    if (!encrypted) return NULL;

    void *plain = NULL;
    size_t plainSize = 0;
    bool decrypted = dpapiUnprotect(encrypted, encryptedSize, &plain, &plainSize);
    free(encrypted);
    if (!decrypted) {
        LOG_ERROR("Twitch: %s belongs to another Windows account or machine", path);
        return NULL;
    }

    char *text = (char *)malloc(plainSize + 1);
    if (text) {
        memcpy(text, plain, plainSize);
        text[plainSize] = '\0';
    }
    dpapiFree(plain);
    return text;
}

// Terminates the line at cursor and returns the start of the next one, or NULL when there is none.
static char *cutLine(char *cursor) {
    char *newline = cursor ? strchr(cursor, '\n') : NULL;
    if (!newline) return NULL;
    *newline = '\0';
    return newline + 1;
}

TwitchClient *twitchAuthLoad(void) {
    char path[TOKEN_PATH_SIZE];
    if (!tokenPath(path, sizeof(path))) return NULL;

    char *blob = decryptBlob(path);
    if (!blob) return NULL;

    char *clientId = blob;
    char *accessToken = cutLine(clientId);
    char *refreshToken = cutLine(accessToken);
    if (!refreshToken) {
        LOG_ERROR("Twitch: %s does not hold a Client-ID and a token pair", path);
        free(blob);
        return NULL;
    }

    TwitchClient *client = twitchCreate(clientId, accessToken);
    if (!client) LOG_ERROR("Twitch: %s holds an unusable Client-ID", path);
    else snprintf(client->refreshToken, sizeof(client->refreshToken), "%s", refreshToken);

    free(blob);
    return client;
}
