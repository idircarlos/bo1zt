#ifndef CLIENT_CUSTOMIZER_H_
#define CLIENT_CUSTOMIZER_H_

#include "client.h"
#include "logic/config.h" // CustomizerConfig

ClientResult clientGetCustomizer(Client *client, CustomizerConfig *out);
ClientResult clientSetCustomizer(Client *client, const CustomizerConfig *config);
ClientResult clientResetCustomizer(Client *client);

#endif // CLIENT_CUSTOMIZER_H_
