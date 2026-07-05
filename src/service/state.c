#include "service/state.h"
#include "service/service_internal.h"
#include "logic/state.h"

#include <string.h>

// Short kebab-case API names for each level, indexed by (Level + 1) so that
// LEVEL_INVALID (-1) maps to index 0.
static const char *LEVEL_NAMES[] = {
    "invalid",       // LEVEL_INVALID
    "main-menu",     // LEVEL_MAIN_MENU
    "kino",          // LEVEL_KINO_DER_TOTEN
    "ascension",     // LEVEL_ASCENSION
    "five",          // LEVEL_FIVE
    "call-of-the-dead", // LEVEL_CALL_OF_THE_DEAD
    "moon",          // LEVEL_MOON
    "shangri-la",    // LEVEL_SHANGRI_LA
    "nacht-der-untoten", // LEVEL_NACH_DER_UNTOTEN
    "der-riese",     // LEVEL_DER_RIESE
    "verruckt",      // LEVEL_VERRUCKT
    "shi-no-numa",   // LEVEL_SHI_NO_NUMA
};

static const char *levelName(Level level) {
    int index = (int)level + 1;
    int count = (int)(sizeof(LEVEL_NAMES) / sizeof(LEVEL_NAMES[0]));
    if (index < 0 || index >= count) return "invalid";
    return LEVEL_NAMES[index];
}

ServiceStateSnapshot serviceStateSnapshot(Service *service) {
    ServiceStateSnapshot snap;
    memset(&snap, 0, sizeof(snap));
    snap.level = "invalid";
    if (!service) return snap;

    State *state = controllerGetState(service->controller);
    if (!state) return snap;

    snap.isGameAttached = state->isGameAttached;
    snap.isZombiesGameOngoing = state->isZombiesGameOngoing;
    snap.isZombiesGamePaused = state->isZombiesGamePaused;
    snap.gameResets = state->gameResets;
    snap.level = levelName(state->activeGame.levelName);
    snap.elapsed = state->activeGame.elapsed;
    snap.movementSpeed = state->activeGame.movementSpeed;
    snap.round = state->activeGame.currentRound.number;
    snap.entitiesCurrent = state->activeGame.currentEntities;
    snap.entitiesMax = state->activeGame.maxEntities;
    return snap;
}
