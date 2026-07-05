#include "service/round.h"
#include "service/service_internal.h"
#include "logic/state.h"
#include "logic/game.h"
#include "logic/game/level.h"
#include "logic/game/round.h"

#include <stdlib.h>
#include <stdio.h>

ServiceResult serviceRoundGet(Service *service, ServiceRoundInfo *infoOut) {
    if (!service || !infoOut) return SERVICE_INVALID_PARAM;
    if (!controllerIsGameAttached(service->controller)) return SERVICE_GAME_NOT_ATTACHED;
    State *state = controllerGetState(service->controller);
    if (!state) return SERVICE_ENGINE_FAILED;
    Round *round = &state->activeGame.currentRound;
    infoOut->number = round->number;
    infoOut->isSpecial = round->isSpecial;
    infoOut->zombiesLeft = round->zombiesLeft;
    return SERVICE_OK;
}

ServiceResult serviceRoundSet(Service *service, int round) {
    if (!service) return SERVICE_INVALID_PARAM;
    if (round < 1) return SERVICE_INVALID_PARAM;
    if (!controllerIsGameAttached(service->controller)) return SERVICE_GAME_NOT_ATTACHED;
    if (!controllerSetRound(service->controller, round)) return SERVICE_ENGINE_FAILED;
    return SERVICE_OK;
}

ServiceResult serviceRoundGetSpecial(Service *service, ServiceSpecialRound *out) {
    if (!service || !out) return SERVICE_INVALID_PARAM;
    if (!controllerIsZombiesGameOngoing(service->controller)) return SERVICE_GAME_NOT_ATTACHED;
    State *state = controllerGetState(service->controller);
    if (!state) return SERVICE_ENGINE_FAILED;
    Game *game = &state->activeGame;

    out->type[0] = '\0';
    out->count = -1;
    out->next[0] = '\0';

    RoundType levelSpecial = levelGetSpecialRound(game->levelName);
    if (levelSpecial == RT_DOGS) snprintf(out->type, sizeof(out->type), "dogs");
    else if (levelSpecial == RT_MONKEYS) snprintf(out->type, sizeof(out->type), "monkeys");
    else if (levelSpecial == RT_THIEF) snprintf(out->type, sizeof(out->type), "thief");

    // Only the DOGS/MONKEYS/THIEF paths produce a heap prediction string (or
    // NULL); the default path returns a literal we must not free, so we compute
    // the count + prediction only when a special type actually applies.
    if (out->type[0] != '\0') {
        int count = 0;
        for (int i = 0; i < game->currentRound.number && i < MAX_ROUNDS; i++) {
            if (game->rounds[i].isSpecial) count++;
        }
        out->count = count;

        const char *next = gameNextPotentialSpecialRounds(game);
        if (next) {
            snprintf(out->next, sizeof(out->next), "%s", next);
            free((void *)next);
        }
    }
    return SERVICE_OK;
}
