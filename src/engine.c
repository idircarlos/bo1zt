#include "engine.h"
#include "engine/memory.h"
#include "engine/gsc.h"
#include "controller.h"
#include "controller/controller_internal.h"
#include "logic/cheat.h"
#include "logger.h"
#include "logic/server.h"
#include <stdlib.h>

struct Engine {
    Controller *controller;
    MemoryBackend *raw;
    GscBackend *gsc;
};

Engine *engineCreate(Controller *controller) {
    Engine *engine = (Engine*)malloc(sizeof(Engine));
    if (!engine) return NULL;
    
    engine->controller = controller;
    engine->raw = memoryBackendCreate(controller);
    engine->gsc = gscBackendCreate(controller);
    
    if (!engine->raw) {
        LOG_ERROR("Couldn't create memory backend");
        free(engine);
        return NULL;
    }
    
    if (!engine->gsc) {
        LOG_ERROR("Couldn't create GSC backend");
        memoryBackendDestroy(engine->raw);
        free(engine);
        return NULL;
    }
    
    return engine;
}

void engineDestroy(Engine *engine) {
    if (engine) {
        if (engine->raw) memoryBackendDestroy(engine->raw);
        if (engine->gsc) gscBackendDestroy(engine->gsc);
        free(engine);
    }
}

// Raw backend forwards

bool engineIsCheatEnabled(Engine *engine, CheatName cheatName) {
    if (!engine) return false;
    
    // Use GSC backend for BOX_NEVER_MOVES
    if (cheatName == CHEAT_NAME_BOX_NEVER_MOVES) {
        return engineGetStaticBox(engine);
    }
    
    return memoryBackendIsCheatEnabled(engine->raw, cheatName);
}

bool engineSetCheatEnabled(Engine *engine, CheatName cheatName, bool enabled) {
    if (!engine) return false;
    
    // Use GSC backend for BOX_NEVER_MOVES
    if (cheatName == CHEAT_NAME_BOX_NEVER_MOVES) {
        return engineSetStaticBox(engine, enabled);
    }
    
    return memoryBackendSetCheatEnabled(engine->raw, cheatName, enabled);
}

bool engineSetSimpleCheat(Engine *engine, SimpleCheatName simpleCheatName, void *value) {
    if (!engine) return false;
    return memoryBackendSetSimpleCheat(engine->raw, simpleCheatName, value);
}

TeleportCoords *engineGetPlayerCurrentCoords(Engine *engine) {
    if (!engine) return NULL;
    return memoryBackendGetPlayerCurrentCoords(engine->raw);
}

bool engineSetRound(Engine *engine, int round) {
    if (!engine) return false;
    int currentRound = gscBackendGetRound(engine->gsc);
    return memoryBackendSetRound(engine->raw, currentRound, round);
}

bool engineIsGameReady(Engine *engine) {
    if (!engine) return false;
    return memoryBackendIsGameReady(engine->raw);
}

Level engineGetLevelName(Engine *engine) {
    if (!engine) return LEVEL_INVALID;
    return memoryBackendGetLevelName(engine->raw);
}

double engineGetLevelElapsedTime(Engine *engine) {
    if (!engine) return 0;
    return memoryBackendGetLevelElapsedTime(engine->raw);
}

float engineGetMovementSpeed(Engine *engine) {
    if (!engine) return 0;
    return memoryBackendGetMovementSpeed(engine->raw);
}

int engineGetSimpleCheatIntValue(Engine *engine, SimpleCheatName simpleCheatName) {
    if (!engine) return 0;
    return memoryBackendGetSimpleCheatIntValue(engine->raw, simpleCheatName);
}

bool engineGetPlayerName(Engine *engine, char *out, size_t size) {
    if (!engine) return false;
    return memoryBackendGetName(engine->raw, out, size);
}

bool engineIsZombiesGameOngoing(Engine *engine) {
    if (!engine) return false;
    return memoryBackendIsZombiesGameOngoing(engine->raw);
}

bool engineIsZombiesGamePaused(Engine *engine) {
    if (!engine) return false;
    return memoryBackendIsZombiesGamePaused(engine->raw);
}

int engineGetGameResets(Engine *engine) {
    if (!engine) return 0;
    return memoryBackendGetGameResets(engine->raw);
}

bool engineSVSendServerCommand(Engine *engine, int commandType, int clientNumber, const char *commands) {
    if (!engine) return false;
    return memoryBackendSVSendServerCommand(engine->raw, commandType, clientNumber, commands);
}

bool engineCBuffAddText(Engine *engine, const char *commands) {
    if (!engine) return false;
    return memoryBackendCBuffAddText(engine->raw, commands);
}

uintptr_t engineGetDVarPointer(Engine *engine, const char *dVar) {
    if (!engine) return 0;
    return memoryBackendGetDVarPointer(engine->raw, dVar);
}

int engineGetClaymoreCount(Engine *engine) {
    if (!engine) return 0;
    return memoryBackendGetClaymoreCount(engine->raw);
}

int engineGetCurrentSnapshotEntities(Engine *engine) {
    if (!engine) return 0;
    return memoryBackendGetCurrentSnapshotEntities(engine->raw);
}

int engineGetMaxSnapshotEntities(Engine *engine) {
    if (!engine) return 0;
    return memoryBackendGetMaxSnapshotEntities(engine->raw);
}

bool engineIsChatOpen(Engine *engine) {
    if (!engine) return false;
    return memoryBackendIsChatOpen(engine->raw);
}

bool engineWriteToChatInput(Engine *engine, const char *text) {
    if (!engine) return false;
    return memoryBackendWriteToChatInput(engine->raw, text);
}

// GSC backend forwards

bool engineAddPerks(Engine *engine, List *perks) {
    if (!engine || !engine->gsc) return false;
    return gscBackendAddPerks(engine->gsc, perks);
}

bool engineRemovePerks(Engine *engine, List *perks) {
    if (!engine || !engine->gsc) return false;
    return gscBackendRemovePerks(engine->gsc, perks);
}

bool engineGetStaticBox(Engine *engine) {
    if (!engine || !engine->gsc) return false;
    return gscBackendGetStaticBox(engine->gsc);
}

bool engineSetStaticBox(Engine *engine, bool enabled) {
    if (!engine || !engine->gsc) return false;
    return gscBackendSetStaticBox(engine->gsc, enabled);
}

bool enginePlayEasterEggSong(Engine *engine) {
    if (!engine || !engine->gsc) return false;
    return gscBackendPlayEasterEggSong(engine->gsc);
}

int engineGetRound(Engine *engine) {
    if (!engine || !engine->gsc) return false;
    return gscBackendGetRound(engine->gsc);
}

bool engineGiveAmmo(Engine *engine) {
    if (!engine) return false;
    return serverExecuteCommand(_controllerGetServer(engine->controller), "give ammo");
}

bool engineGiveWeapons(Engine *engine, List *weapons) {
    if (!engine || !engine->gsc) return false;
    return gscBackendGiveWeapons(engine->gsc, weapons);
}

bool engineTakeWeapons(Engine *engine) {
    if (!engine || !engine->gsc) return false;
    return gscBackendTakeWeapons(engine->gsc);
}
