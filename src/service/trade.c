#include "service/trade.h"
#include "service/service_internal.h"
#include "logic/state.h"
#include "logic/game.h"
#include "logic/game/trade.h"

// The service manipulates activeGame.currentTrade directly, exactly as the
// /trade chat command does (src/logic/command/misc.c), using the level elapsed
// time as the clock source.

static Game *activeGame(Service *service) {
    State *state = controllerGetState(service->controller);
    return state ? &state->activeGame : NULL;
}

ServiceResult serviceTradeStatus(Service *service, ServiceTradeStatus *out) {
    if (!service || !out) return SERVICE_INVALID_PARAM;
    if (!controllerIsZombiesGameOngoing(service->controller)) return SERVICE_GAME_NOT_ATTACHED;
    Game *game = activeGame(service);
    if (!game) return SERVICE_ENGINE_FAILED;
    Trade *trade = &game->currentTrade;
    int now = controllerGetLevelElapsedTime(service->controller);
    out->running = tradeRunning(trade);
    out->elapsedMs = out->running ? tradeGetElapsed(trade, now) : 0;
    out->hits = tradeGetHits(trade);
    return SERVICE_OK;
}

ServiceResult serviceTradeTotal(Service *service, ServiceTradeTotal *out) {
    if (!service || !out) return SERVICE_INVALID_PARAM;
    if (!controllerIsZombiesGameOngoing(service->controller)) return SERVICE_GAME_NOT_ATTACHED;
    Game *game = activeGame(service);
    if (!game) return SERVICE_ENGINE_FAILED;
    out->trades = game->tradeCount;
    out->totalMs = 0;
    out->totalHits = 0;
    for (int i = 0; i < game->tradeCount; i++) {
        out->totalMs += game->trades[i].endTimestamp - game->trades[i].startTimestamp;
        out->totalHits += game->trades[i].hits;
    }
    return SERVICE_OK;
}

ServiceResult serviceTradeStart(Service *service) {
    if (!service) return SERVICE_INVALID_PARAM;
    if (!controllerIsZombiesGameOngoing(service->controller)) return SERVICE_GAME_NOT_ATTACHED;
    Game *game = activeGame(service);
    if (!game) return SERVICE_ENGINE_FAILED;
    int now = controllerGetLevelElapsedTime(service->controller);
    if (!tradeStart(&game->currentTrade, now)) return SERVICE_INVALID_PARAM; // already running
    return SERVICE_OK;
}

ServiceResult serviceTradeEnd(Service *service, ServiceTradeStatus *out) {
    if (!service || !out) return SERVICE_INVALID_PARAM;
    if (!controllerIsZombiesGameOngoing(service->controller)) return SERVICE_GAME_NOT_ATTACHED;
    Game *game = activeGame(service);
    if (!game) return SERVICE_ENGINE_FAILED;
    Trade *trade = &game->currentTrade;
    int now = controllerGetLevelElapsedTime(service->controller);
    if (!tradeEnd(trade, now)) return SERVICE_INVALID_PARAM; // none running

    out->running = false;
    out->elapsedMs = tradeGetElapsed(trade, now);
    out->hits = tradeGetHits(trade);

    if (game->tradeCount < MAX_TRADES) {
        game->trades[game->tradeCount++] = *trade;
    }
    tradeClear(trade);
    return SERVICE_OK;
}

ServiceResult serviceTradeCancel(Service *service) {
    if (!service) return SERVICE_INVALID_PARAM;
    if (!controllerIsZombiesGameOngoing(service->controller)) return SERVICE_GAME_NOT_ATTACHED;
    Game *game = activeGame(service);
    if (!game) return SERVICE_ENGINE_FAILED;
    if (!tradeCancel(&game->currentTrade)) return SERVICE_INVALID_PARAM; // none running
    return SERVICE_OK;
}
