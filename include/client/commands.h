#ifndef CLIENT_COMMANDS_H_
#define CLIENT_COMMANDS_H_

#include "client.h"

typedef struct {
    char name[64];
    char usage[128];
    char description[256];
} CommandInfo;

ClientResult clientGetCommands(Client *client, CommandInfo *out, int max, int *countOut);

#endif // CLIENT_COMMANDS_H_
