#include "service/stats.h"
#include "service/service_internal.h"
#include "logic/state.h"
#include "logic/game.h"
#include "logic/game/level.h"
#include "logic/game/round.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

static ServiceResult resolveGame(Service *service, Game **gameOut) {
    if (!service) return SERVICE_INVALID_PARAM;
    if (!controllerIsZombiesGameOngoing(service->controller)) return SERVICE_GAME_NOT_ATTACHED;
    State *state = controllerGetState(service->controller);
    if (!state) return SERVICE_ENGINE_FAILED;
    *gameOut = &state->activeGame;
    return SERVICE_OK;
}

// Seconds-per-horde for a single completed round, unrounded.
static double roundSph(Round *r) {
    int elapsedSeconds = (r->endTimestamp - r->startTimestamp) / 1000;
    float hordeCount = roundHordeCount(r);
    if (hordeCount < 1.0f) hordeCount = 1.0f;
    return (double)elapsedSeconds / hordeCount;
}

static ServiceResult computeSph(Game *game, int scopeRound, double *sphOut) {
    RoundType levelSpecial = levelGetSpecialRound(game->levelName);
    if (scopeRound > 0) {
        if (scopeRound >= game->currentRound.number) return SERVICE_INVALID_PARAM;
        Round *r = &game->rounds[scopeRound - 1];
        if (r->isSpecial && levelSpecial != RT_GEORGE) return SERVICE_INVALID_PARAM;
        *sphOut = round(roundSph(r) * 10.0) / 10.0;
        return SERVICE_OK;
    }
    double total = 0.0;
    int valid = 0;
    for (int i = 0; i < game->currentRound.number - 1 && i < MAX_ROUNDS; i++) {
        Round *r = &game->rounds[i];
        if (r->isSpecial && levelSpecial != RT_GEORGE) continue;
        total += roundSph(r);
        valid++;
    }
    *sphOut = (valid > 0) ? round((total / valid) * 10.0) / 10.0 : 0.0;
    return SERVICE_OK;
}

static void computeSpecialRounds(Game *game, ServiceSpecialRounds *out) {
    RoundType levelSpecial = levelGetSpecialRound(game->levelName);
    out->dogs = (levelSpecial == RT_DOGS) ? 0 : -1;
    out->monkeys = (levelSpecial == RT_MONKEYS) ? 0 : -1;
    out->thief = (levelSpecial == RT_THIEF) ? 0 : -1;

    int specialCount = 0;
    for (int i = 0; i < game->currentRound.number && i < MAX_ROUNDS; i++) {
        if (game->rounds[i].isSpecial) specialCount++;
    }
    if (levelSpecial == RT_DOGS) out->dogs = specialCount;
    else if (levelSpecial == RT_MONKEYS) out->monkeys = specialCount;
    else if (levelSpecial == RT_THIEF) out->thief = specialCount;

    // Prediction of the map's next special-round numbers. Only the
    // DOGS/MONKEYS/THIEF paths return a heap string (or NULL); the default path
    // returns a string literal we must not free, so restrict the call.
    out->next[0] = '\0';
    if (levelSpecial == RT_DOGS || levelSpecial == RT_MONKEYS || levelSpecial == RT_THIEF) {
        const char *next = gameNextPotentialSpecialRounds(game);
        if (next) {
            snprintf(out->next, sizeof(out->next), "%s", next);
            free((void *)next);
        }
    }
}

ServiceResult serviceStatsGet(Service *service, int scopeRound, ServiceStats *out) {
    if (!out) return SERVICE_INVALID_PARAM;
    Game *game;
    ServiceResult r = resolveGame(service, &game);
    if (r != SERVICE_OK) return r;

    out->entitiesCurrent = game->currentEntities;
    out->entitiesMax = game->maxEntities;
    out->claymores = controllerGetClaymoreCount(service->controller);
    out->revives = gameGetQuickRevivesDrunk(game);

    ServiceSpecialRounds special;
    computeSpecialRounds(game, &special);
    out->specialDogs = special.dogs;
    out->specialMonkeys = special.monkeys;
    out->specialThief = special.thief;
    snprintf(out->nextSpecialRounds, sizeof(out->nextSpecialRounds), "%s", special.next);

    return computeSph(game, scopeRound, &out->sph);
}

ServiceResult serviceStatsGetSph(Service *service, int scopeRound, double *sphOut) {
    if (!sphOut) return SERVICE_INVALID_PARAM;
    Game *game;
    ServiceResult r = resolveGame(service, &game);
    if (r != SERVICE_OK) return r;
    return computeSph(game, scopeRound, sphOut);
}

ServiceResult serviceStatsGetClaymores(Service *service, int *claymoresOut) {
    if (!claymoresOut) return SERVICE_INVALID_PARAM;
    Game *game;
    ServiceResult r = resolveGame(service, &game);
    if (r != SERVICE_OK) return r;
    (void)game;
    *claymoresOut = controllerGetClaymoreCount(service->controller);
    return SERVICE_OK;
}

ServiceResult serviceStatsGetEntities(Service *service, int *currentOut, int *maxOut) {
    if (!currentOut || !maxOut) return SERVICE_INVALID_PARAM;
    Game *game;
    ServiceResult r = resolveGame(service, &game);
    if (r != SERVICE_OK) return r;
    *currentOut = game->currentEntities;
    *maxOut = game->maxEntities;
    return SERVICE_OK;
}

ServiceResult serviceStatsGetRevives(Service *service, int *revivesOut) {
    if (!revivesOut) return SERVICE_INVALID_PARAM;
    Game *game;
    ServiceResult r = resolveGame(service, &game);
    if (r != SERVICE_OK) return r;
    *revivesOut = gameGetQuickRevivesDrunk(game);
    return SERVICE_OK;
}
