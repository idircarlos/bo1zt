#ifndef CLIENT_BINDS_H_
#define CLIENT_BINDS_H_

#include "client.h"
#include "logic/config.h" // BindsConfig

ClientResult clientGetBinds(Client *client, BindsConfig *out);
ClientResult clientSetBinds(Client *client, const BindsConfig *config);
ClientResult clientResetBinds(Client *client);

#endif // CLIENT_BINDS_H_
