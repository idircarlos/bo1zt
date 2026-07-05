#ifndef CLIENT_H_
#define CLIENT_H_

#include <stdbool.h>
#include <stddef.h>

typedef struct Client Client;

typedef enum {
    CLIENT_OK = 0,
    CLIENT_ERR_UNREACHABLE,   // could not reach the daemon (transport failure)
    CLIENT_ERR_INVALID_PARAM, // 400 Bad Request
    CLIENT_ERR_NOT_FOUND,     // 404 (or an unknown enum resolved locally)
    CLIENT_ERR_CONFLICT,      // 409 (game not attached / ready / ongoing)
    CLIENT_ERR_ENGINE,        // 500 engine/API failure
    CLIENT_ERR_PROTOCOL,      // 2xx but the body was missing/malformed
} ClientResult;

Client *clientCreate(int port);
void    clientDestroy(Client *client);

const char *clientLastErrorCode(const Client *client);
const char *clientLastErrorMessage(const Client *client);

ClientResult clientGetVersion(Client *client, char *out, size_t size);

#endif // CLIENT_H_
