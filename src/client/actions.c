#include "client/actions.h"
#include "client/client_internal.h"

ClientResult clientPlayMusic(Client *client) {
    return clientRequest(client, "POST", CLIENT_API_BASE "/misc/music", NULL, NULL);
}
