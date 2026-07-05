#include "service/cheats.h"
#include "service/service_internal.h"
#include "logic/cheat.h"
#include "logic/cheat/manager/actions.h"

#include <string.h>

// Maps the API's kebab-case cheat names to the engine's CheatName enum.
typedef struct {
    const char *name;
    CheatName cheat;
} CheatEntry;

static const CheatEntry CHEAT_TABLE[] = {
    { "god",                  CHEAT_NAME_GOD_MODE },
    { "noclip",               CHEAT_NAME_NO_CLIP },
    { "invisible",            CHEAT_NAME_INVISIBLE },
    { "infinite-ammo",        CHEAT_NAME_INFINITE_AMMO },
    { "instant-kill",         CHEAT_NAME_INSTANT_KILL },
    { "no-recoil",            CHEAT_NAME_NO_RECOIL },
    { "small-crosshair",      CHEAT_NAME_SMALL_CROSSHAIR },
    { "fast-gameplay",        CHEAT_NAME_FAST_GAMEPLAY },
    { "no-shellshock",        CHEAT_NAME_NO_SHELLSHOCK },
    { "increase-knife-range", CHEAT_NAME_INCREASE_KNIFE_RANGE },
    { "static-box",           CHEAT_NAME_BOX_NEVER_MOVES },
    { "third-person",         CHEAT_NAME_THIRD_PERSON },
    { "borderless",           CHEAT_NAME_MAKE_BORDERLESS },
    { "unlimit-fps",          CHEAT_NAME_UNLIMIT_FPS },
    { "disable-hud",          CHEAT_NAME_DISABLE_HUD },
    { "disable-fog",          CHEAT_NAME_DISABLE_FOG },
    { "fullbright",           CHEAT_NAME_FULLBRIGHT },
    { "colorized",            CHEAT_NAME_COLORIZED },
    { "show-fps",             CHEAT_NAME_SHOW_FPS },
    { "fix-movement-speed",   CHEAT_NAME_FIX_MOVEMENT_SPEED },
};

static const int CHEAT_TABLE_SIZE = (int)(sizeof(CHEAT_TABLE) / sizeof(CHEAT_TABLE[0]));

static const CheatEntry *findCheat(const char *name) {
    if (!name) return NULL;
    for (int i = 0; i < CHEAT_TABLE_SIZE; i++) {
        if (strcmp(CHEAT_TABLE[i].name, name) == 0) {
            return &CHEAT_TABLE[i];
        }
    }
    return NULL;
}

int serviceCheatCount(void) {
    return CHEAT_TABLE_SIZE;
}

const char *serviceCheatNameAt(int index) {
    if (index < 0 || index >= CHEAT_TABLE_SIZE) return NULL;
    return CHEAT_TABLE[index].name;
}

bool serviceCheatExists(const char *name) {
    return findCheat(name) != NULL;
}

// The daemon owns config, so /cheats reflects the *desired* (persisted) state,
// not live game memory. Reads and writes both go through the cheat manager:
// the config value is always readable and writable regardless of whether a game
// is attached, and the manager applies the cheat live (and re-applies on
// attach) when conditions allow. This keeps /god (chat), `bo1zt god` (CLI), the
// GUI checkbox and PUT /cheats/god on one path.

ServiceResult serviceCheatGet(Service *service, const char *name, bool *enabledOut) {
    if (!service || !enabledOut) return SERVICE_INVALID_PARAM;
    const CheatEntry *entry = findCheat(name);
    if (!entry) return SERVICE_NOT_FOUND;
    CheatManager *manager = controllerGetCheatManager(service->controller);
    if (!manager) return SERVICE_ENGINE_FAILED;
    *enabledOut = cheatManagerGetToggle(manager, entry->cheat);
    return SERVICE_OK;
}

ServiceResult serviceCheatSet(Service *service, const char *name, bool enabled, bool *enabledOut) {
    if (!service) return SERVICE_INVALID_PARAM;
    const CheatEntry *entry = findCheat(name);
    if (!entry) return SERVICE_NOT_FOUND;
    CheatManager *manager = controllerGetCheatManager(service->controller);
    if (!manager) return SERVICE_ENGINE_FAILED;

    // OK / NO_CHANGE / CONDITION_NOT_MET all mean the desired state was recorded
    // and persisted; only a hard engine/API failure is an error. When
    // conditions aren't met yet (game not ready) the manager still saved the
    // config and will apply it live on attach.
    CheatResult r = cheatManagerSetToggle(manager, entry->cheat, enabled);
    if (r == CHEAT_RESULT_API_FAILED) return SERVICE_ENGINE_FAILED;
    if (enabledOut) *enabledOut = enabled;
    return SERVICE_OK;
}
