#include "logic/cheat/manager/state.h"
#include <string.h>

void appliedStateClear(AppliedState *applied) {
    memset(applied, 0, sizeof(AppliedState));
}

bool getConfigToggleValue(Config *config, CheatName cheat) {
    switch (cheat) {
        // HacksConfig
        case CHEAT_NAME_GOD_MODE: return config->hacks.godMode;
        case CHEAT_NAME_NO_CLIP: return config->hacks.noClip;
        case CHEAT_NAME_INVISIBLE: return config->hacks.invisible;
        case CHEAT_NAME_INFINITE_AMMO: return config->hacks.infiniteAmmo;
        case CHEAT_NAME_INSTANT_KILL: return config->hacks.instantKill;
        case CHEAT_NAME_NO_RECOIL: return config->hacks.noRecoil;
        case CHEAT_NAME_SMALL_CROSSHAIR: return config->hacks.smallCrosshair;
        case CHEAT_NAME_FAST_GAMEPLAY: return config->hacks.fastGameplay;
        case CHEAT_NAME_NO_SHELLSHOCK: return config->hacks.noShellshock;
        case CHEAT_NAME_INCREASE_KNIFE_RANGE: return config->hacks.increaseKnifeRange;
        case CHEAT_NAME_BOX_NEVER_MOVES: return config->hacks.boxNeverMoves;
        case CHEAT_NAME_THIRD_PERSON: return config->hacks.thirdPerson;
        // GraphicsConfig
        case CHEAT_NAME_MAKE_BORDERLESS: return config->graphics.borderless;
        case CHEAT_NAME_UNLIMIT_FPS: return config->graphics.unlimitFps;
        case CHEAT_NAME_DISABLE_HUD: return config->graphics.disableHud;
        case CHEAT_NAME_DISABLE_FOG: return config->graphics.disableFog;
        case CHEAT_NAME_FULLBRIGHT: return config->graphics.fullbright;
        case CHEAT_NAME_COLORIZED: return config->graphics.colorized;
        // GameConfig
        case CHEAT_NAME_FIX_MOVEMENT_SPEED: return config->game.fixMovementSpeed;
        case CHEAT_NAME_SHOW_FPS: return config->game.showFps;
        default: return false;
    }
}

void setConfigToggleValue(Config *config, CheatName cheat, bool value) {
    switch (cheat) {
        // HacksConfig
        case CHEAT_NAME_GOD_MODE: config->hacks.godMode = value; break;
        case CHEAT_NAME_NO_CLIP: config->hacks.noClip = value; break;
        case CHEAT_NAME_INVISIBLE: config->hacks.invisible = value; break;
        case CHEAT_NAME_INFINITE_AMMO: config->hacks.infiniteAmmo = value; break;
        case CHEAT_NAME_INSTANT_KILL: config->hacks.instantKill = value; break;
        case CHEAT_NAME_NO_RECOIL: config->hacks.noRecoil = value; break;
        case CHEAT_NAME_SMALL_CROSSHAIR: config->hacks.smallCrosshair = value; break;
        case CHEAT_NAME_FAST_GAMEPLAY: config->hacks.fastGameplay = value; break;
        case CHEAT_NAME_NO_SHELLSHOCK: config->hacks.noShellshock = value; break;
        case CHEAT_NAME_INCREASE_KNIFE_RANGE: config->hacks.increaseKnifeRange = value; break;
        case CHEAT_NAME_BOX_NEVER_MOVES: config->hacks.boxNeverMoves = value; break;
        case CHEAT_NAME_THIRD_PERSON: config->hacks.thirdPerson = value; break;
        // GraphicsConfig
        case CHEAT_NAME_MAKE_BORDERLESS: config->graphics.borderless = value; break;
        case CHEAT_NAME_UNLIMIT_FPS: config->graphics.unlimitFps = value; break;
        case CHEAT_NAME_DISABLE_HUD: config->graphics.disableHud = value; break;
        case CHEAT_NAME_DISABLE_FOG: config->graphics.disableFog = value; break;
        case CHEAT_NAME_FULLBRIGHT: config->graphics.fullbright = value; break;
        case CHEAT_NAME_COLORIZED: config->graphics.colorized = value; break;
        // GameConfig
        case CHEAT_NAME_FIX_MOVEMENT_SPEED: config->game.fixMovementSpeed = value; break;
        case CHEAT_NAME_SHOW_FPS: config->game.showFps = value; break;
        default: break;
    }
}

bool getAppliedToggleValue(AppliedState *applied, CheatName cheat) {
    switch (cheat) {
        // HacksConfig
        case CHEAT_NAME_GOD_MODE: return applied->godMode;
        case CHEAT_NAME_NO_CLIP: return applied->noClip;
        case CHEAT_NAME_INVISIBLE: return applied->invisible;
        case CHEAT_NAME_INFINITE_AMMO: return applied->infiniteAmmo;
        case CHEAT_NAME_INSTANT_KILL: return applied->instantKill;
        case CHEAT_NAME_NO_RECOIL: return applied->noRecoil;
        case CHEAT_NAME_SMALL_CROSSHAIR: return applied->smallCrosshair;
        case CHEAT_NAME_FAST_GAMEPLAY: return applied->fastGameplay;
        case CHEAT_NAME_NO_SHELLSHOCK: return applied->noShellshock;
        case CHEAT_NAME_INCREASE_KNIFE_RANGE: return applied->increaseKnifeRange;
        case CHEAT_NAME_BOX_NEVER_MOVES: return applied->boxNeverMoves;
        case CHEAT_NAME_THIRD_PERSON: return applied->thirdPerson;
        // GraphicsConfig
        case CHEAT_NAME_MAKE_BORDERLESS: return applied->borderless;
        case CHEAT_NAME_UNLIMIT_FPS: return applied->unlimitFps;
        case CHEAT_NAME_DISABLE_HUD: return applied->disableHud;
        case CHEAT_NAME_DISABLE_FOG: return applied->disableFog;
        case CHEAT_NAME_FULLBRIGHT: return applied->fullbright;
        case CHEAT_NAME_COLORIZED: return applied->colorized;
        // GameConfig
        case CHEAT_NAME_FIX_MOVEMENT_SPEED: return applied->fixMovementSpeed;
        case CHEAT_NAME_SHOW_FPS: return applied->showFps;
        default: return false;
    }
}

void setAppliedToggleValue(AppliedState *applied, CheatName cheat, bool value) {
    switch (cheat) {
        // HacksConfig
        case CHEAT_NAME_GOD_MODE: applied->godMode = value; break;
        case CHEAT_NAME_NO_CLIP: applied->noClip = value; break;
        case CHEAT_NAME_INVISIBLE: applied->invisible = value; break;
        case CHEAT_NAME_INFINITE_AMMO: applied->infiniteAmmo = value; break;
        case CHEAT_NAME_INSTANT_KILL: applied->instantKill = value; break;
        case CHEAT_NAME_NO_RECOIL: applied->noRecoil = value; break;
        case CHEAT_NAME_SMALL_CROSSHAIR: applied->smallCrosshair = value; break;
        case CHEAT_NAME_FAST_GAMEPLAY: applied->fastGameplay = value; break;
        case CHEAT_NAME_NO_SHELLSHOCK: applied->noShellshock = value; break;
        case CHEAT_NAME_INCREASE_KNIFE_RANGE: applied->increaseKnifeRange = value; break;
        case CHEAT_NAME_BOX_NEVER_MOVES: applied->boxNeverMoves = value; break;
        case CHEAT_NAME_THIRD_PERSON: applied->thirdPerson = value; break;
        // GraphicsConfig
        case CHEAT_NAME_MAKE_BORDERLESS: applied->borderless = value; break;
        case CHEAT_NAME_UNLIMIT_FPS: applied->unlimitFps = value; break;
        case CHEAT_NAME_DISABLE_HUD: applied->disableHud = value; break;
        case CHEAT_NAME_DISABLE_FOG: applied->disableFog = value; break;
        case CHEAT_NAME_FULLBRIGHT: applied->fullbright = value; break;
        case CHEAT_NAME_COLORIZED: applied->colorized = value; break;
        // GameConfig
        case CHEAT_NAME_FIX_MOVEMENT_SPEED: applied->fixMovementSpeed = value; break;
        case CHEAT_NAME_SHOW_FPS: applied->showFps = value; break;
        default: break;
    }
}
