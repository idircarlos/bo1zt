#ifndef CLIENT_GAME_H_
#define CLIENT_GAME_H_

#include "client.h"

typedef struct {
    bool attached;
    bool running;
    bool ready;
    bool windowFocused;
    bool dllInjected;
} GameStatus;

ClientResult clientGetGameStatus(Client *client, GameStatus *out);

ClientResult clientLaunchGame(Client *client);
ClientResult clientCloseGame(Client *client);
ClientResult clientRestartGame(Client *client);

typedef struct {
    char location[256];
    char hostname[256];
    char character[16];  // kebab-case; persisted, readable while detached
} GameConfigInfo;

ClientResult clientGetGameConfig(Client *client, GameConfigInfo *out);
ClientResult clientSetGameLocation(Client *client, const char *location);
ClientResult clientSetGameHostname(Client *client, const char *hostname);
// Set the persisted character (kebab-case). Works whether or not a game is
// attached (unlike clientSetPlayerCharacter, which gates on an ongoing game).
ClientResult clientSetGameCharacter(Client *client, const char *character);

#endif // CLIENT_GAME_H_
