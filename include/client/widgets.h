#ifndef CLIENT_WIDGETS_H_
#define CLIENT_WIDGETS_H_

#include "client.h"
#include "logic/config.h" // WidgetConfig

int clientWidgetCount(void);
const char *clientWidgetNameAt(int index);

ClientResult clientGetWidget(Client *client, const char *name, WidgetConfig *out);
ClientResult clientSetWidget(Client *client, const char *name, const WidgetConfig *config);
ClientResult clientResetWidget(Client *client, const char *name);

#endif // CLIENT_WIDGETS_H_
