#ifndef SERVICE_GAME_H_
#define SERVICE_GAME_H_

#include "service.h"

typedef struct {
    bool attached;
    bool running;
    bool ready;
    bool windowFocused;
    bool dllInjected;
} ServiceGameStatus;

ServiceGameStatus serviceGameStatus(Service *service);

ServiceResult serviceGameLaunch(Service *service);
ServiceResult serviceGameClose(Service *service);
ServiceResult serviceGameRestart(Service *service);

typedef struct {
    char location[256];
    char hostname[256];
    char character[16];  // kebab-case; persisted config, readable while detached
} ServiceGameConfig;

typedef struct {
    bool hasLocation;  char location[256];
    bool hasHostname;  char hostname[256];
    bool hasCharacter; char character[16];
} ServiceGameConfigPatch;

ServiceResult serviceGameGetConfig(Service *service, ServiceGameConfig *out);
// location is written to config; hostname goes through the cheat manager
// (persist + apply live), matching the GUI's old path.
ServiceResult serviceGameUpdateConfig(Service *service, const ServiceGameConfigPatch *patch);

#endif // SERVICE_GAME_H_
