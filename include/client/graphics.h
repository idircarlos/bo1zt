#ifndef CLIENT_GRAPHICS_H_
#define CLIENT_GRAPHICS_H_

#include "client.h"
#include "logic/config.h" // GraphicsConfig

ClientResult clientGetGraphics(Client *client, GraphicsConfig *out);
ClientResult clientSetGraphics(Client *client, const GraphicsConfig *config);
ClientResult clientResetGraphics(Client *client);

#endif // CLIENT_GRAPHICS_H_
