#ifndef CHEAT_MANAGER_STATE_H_
#define CHEAT_MANAGER_STATE_H_

#include <stdbool.h>
#include "logic/cheat.h"
#include "logic/config.h"
#include "logic/server.h"

typedef struct {
    // HacksConfig toggles
    bool godMode;
    bool noClip;
    bool invisible;
    bool infiniteAmmo;
    bool instantKill;
    bool noRecoil;
    bool smallCrosshair;
    bool fastGameplay;
    bool noShellshock;
    bool increaseKnifeRange;
    bool boxNeverMoves;
    bool thirdPerson;
    // GraphicsConfig toggles
    bool borderless;
    bool unlimitFps;
    bool disableHud;
    bool disableFog;
    bool fullbright;
    bool colorized;
    // GameConfig toggles
    bool fixMovementSpeed;
    bool showFps;
    // Value cheats
    int fov;
    int fovScale;
    int fpsCap;
    char hostname[256];
    int character;
    ChatColor chatNameColor;
} AppliedState;

void appliedStateClear(AppliedState *applied);
bool getConfigToggleValue(Config *config, CheatName cheat);
void setConfigToggleValue(Config *config, CheatName cheat, bool value);
bool getAppliedToggleValue(AppliedState *applied, CheatName cheat);
void setAppliedToggleValue(AppliedState *applied, CheatName cheat, bool value);

#endif // CHEAT_MANAGER_STATE_H_
