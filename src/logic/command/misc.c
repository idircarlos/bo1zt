#include "logic/command/misc.h"
#include "api.h"
#include "controller/controller_internal.h"
#include "logic/cheat.h"
#include "logic/game/perk.h"
#include "logic/game/level.h"
#include "logic/game/trade.h"
#include "logic/game.h"
#include "logic/server.h"
#include "logic/state.h"
#include "logger.h"
#include "utils/list.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static Controller *controller;
static Server *server;
static Api *api;

void commandMiscInit(Controller *controllerInstance) {
    controller = controllerInstance;
    server = _controllerGetServer(controller);
    api = _controllerGetApi(controller);
}

static Perk getPerkFromAbbreviation(const char *perkAbbreviation) {
    if (strcmp("qr", perkAbbreviation) == 0) return PERK_QUICK_REVIVE;
    if (strcmp("jg", perkAbbreviation) == 0) return PERK_JUGGERNAUT;
    if (strcmp("sc", perkAbbreviation) == 0) return PERK_SPEED_COLA;
    if (strcmp("dt", perkAbbreviation) == 0) return PERK_DOUBLE_TAP;
    if (strcmp("su", perkAbbreviation) == 0) return PERK_STAMINA_UP;
    if (strcmp("mk", perkAbbreviation) == 0) return PERK_MULE_KICK;
    return PERK_INVALID;
}

static List *buildPerkList(Command command) {
    List *perks = listCreate();
    for (int i = 2; i < command.argc; i++) {
        Perk perk = getPerkFromAbbreviation(command.argv[i]);
        if (perk == PERK_INVALID) {
            LOG_WARN("Invalid perk abbreviation: %s\n", command.argv[i]);
            continue;
        }
        listAddInt(perks, perk);
    }
    return perks;
}

bool commandPerkHandle(Command command) {
    if (command.argc < 3) {
        serverChatMessage(server, "Usage: /perk <add | rm> <perk1> [perk2]...");
        serverChatMessage(server, "Perks: jg qr sc dt su mk");
        return false;
    }

    List *perks = buildPerkList(command);
    if (listIsEmpty(perks)) {
        listDestroy(perks);
        return false;
    }

    bool success = false;
    if (strcmp(command.argv[1], "add") == 0) {
        success = apiAddPerks(api, perks);
    } else if (strcmp(command.argv[1], "rm") == 0) {
        success = apiRemovePerks(api, perks);
    } else {
        serverChatMessage(server, "Unknown perk action. Use: add, rm");
    }

    listDestroy(perks);
    return success;
}

bool commandGiveHandle(Command command) {
    if (command.argc < 2) {
        serverChatMessage(server, "/give must receive an argument!");
        serverChatMessage(server, "Usage: /give ammo | <weapon>");
        return false;
    }

    char buffer[64];
    snprintf(buffer, 64, "give %s", command.argv[1]);
    serverExecuteCommand(server, buffer);
    return true;
}

bool commandTpHandle(Command command) {
    char buffer[64];

    if (command.argc == 1) {
        TeleportCoords *coords = controllerGetPlayerCurrentCoords(controller);
        if (coords) {
            snprintf(buffer, 64, "Position: (%d, %d, %d)", (int)coords->x, (int)coords->y, (int)coords->z);
            serverChatMessage(server, buffer);
            return true;
        }
        serverChatMessage(server, "Failed to get current position!");
        return false;
    }

    if (command.argc < 4) {
        serverChatMessage(server, "Usage: /tp [x y z]");
        return false;
    }

    float x = (float)atof(command.argv[1]);
    float y = (float)atof(command.argv[2]);
    float z = (float)atof(command.argv[3]);

    TeleportCoords coords = { x, y, z };
    bool success = controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_TELEPORT, &coords);

    if (!success) {
        serverChatMessage(server, "Failed to teleport!");
    }

    return success;
}

bool commandNextSpecialRoundHandle(Command command) {
    (void)command;
    State *state = controllerGetState(controller);
    Game *game = &state->activeGame;
    const char *result = gameNextPotentialSpecialRounds(game);
    if (result) {
        serverChatMessage(server, result);
        free((void*)result);
        return true;
    }
    serverChatMessage(server, "No special rounds");
    return false;
}

bool commandClaymoresHandle(Command command) {
    (void)command;
    int count = apiGetClaymoreCount(api);
    char buffer[64];
    snprintf(buffer, 64, "Claymores: %d", count);
    serverChatMessage(server, buffer);
    return true;
}

bool commandEntitiesHandle(Command command) {
    (void)command;
    State *state = controllerGetState(controller);
    Game *game = &state->activeGame;
    char buffer[64];
    snprintf(buffer, 64, "Entities: %d/%d", game->currentEntities, game->maxEntities);
    serverChatMessage(server, buffer);
    return true;
}

bool commandSphHandle(Command command) {
    State *state = controllerGetState(controller);
    Game *game = &state->activeGame;
    char buffer[128];

    // Check if there are any completed rounds
    if (game->currentRound.number <= 1) {
        serverChatMessage(server, "No previous rounds to display SPH for!");
        return false;
    }

    RoundType levelSpecial = levelGetSpecialRound(game->levelName);

    // If argument provided, get specific round
    if (command.argc >= 2) {
        int roundNum = atoi(command.argv[1]);
        if (roundNum <= 0 || roundNum >= game->currentRound.number) {
            snprintf(buffer, 128, "Usage: /sph or /sph <1-%d>", game->currentRound.number - 1);
            serverChatMessage(server, buffer);
            return false;
        }
        
        Round *targetRound = &game->rounds[roundNum - 1];
        
        // Check if it's a special round (except George rounds)
        if (targetRound->isSpecial && levelSpecial != RT_GEORGE) {
            snprintf(buffer, 128, "Round %d was a special round!", targetRound->number);
            serverChatMessage(server, buffer);
            return true;
        }

        // Calculate SPH for specific round
        int elapsedSeconds = (targetRound->endTimestamp - targetRound->startTimestamp) / 1000;
        float hordeCount = roundHordeCount(targetRound);
        if (hordeCount < 1.0f) hordeCount = 1.0f;

        double sph = round((double)elapsedSeconds / hordeCount * 10.0) / 10.0;
        snprintf(buffer, 128, "SPH for round %d: %.1f", targetRound->number, sph);
        serverChatMessage(server, buffer);
        return true;
    }

    // No argument: calculate average SPH across all completed rounds
    double totalSph = 0.0;
    int validRounds = 0;

    for (int i = 0; i < game->currentRound.number - 1; i++) {
        Round *r = &game->rounds[i];
        
        // Skip special rounds (except George)
        if (r->isSpecial && levelSpecial != RT_GEORGE) continue;

        int elapsedSeconds = (r->endTimestamp - r->startTimestamp) / 1000;
        float hordeCount = roundHordeCount(r);
        if (hordeCount < 1.0f) hordeCount = 1.0f;

        totalSph += (double)elapsedSeconds / hordeCount;
        validRounds++;
    }

    if (validRounds == 0) {
        serverChatMessage(server, "No valid rounds to calculate SPH yet!");
        return false;
    }

    double avgSph = round((totalSph / validRounds) * 10.0) / 10.0;
    snprintf(buffer, 128, "Average SPH: %.2f", avgSph);
    serverChatMessage(server, buffer);
    return true;
}

bool commandRestartHandle(Command command) {
    (void)command;
    return serverExecuteCommand(server, "map_restart");
}

bool commandMusicHandle(Command command) {
    (void)command;
    return apiPlayEasterEggSong(api);
}

bool commandTradeHandle(Command command) {
    State *state = controllerGetState(controller);
    Game *game = &state->activeGame;
    Trade *trade = &game->currentTrade;
    int timestamp = controllerGetLevelElapsedTime(controller);
    char buffer[128];

    if (command.argc < 2) {
        if (!tradeRunning(trade)) {
            serverChatMessage(server, "No trade running");
            return true;
        }
        int ms = tradeGetElapsed(trade, timestamp);
        int secs = ms / 1000;
        int h = secs / 3600, m = (secs % 3600) / 60, s = secs % 60;
        snprintf(buffer, 128, "Trade: %02d:%02d:%02d | Hits: %d", h, m, s, tradeGetHits(trade));
        serverChatMessage(server, buffer);
        return true;
    }

    if (strcmp(command.argv[1], "start") == 0) {
        if (tradeStart(trade, timestamp)) {
            serverChatMessage(server, "Trade started");
        } else {
            serverChatMessage(server, "Trade already running");
        }
        return true;
    }

    if (strcmp(command.argv[1], "end") == 0) {
        if (tradeEnd(trade, timestamp)) {
            int ms = tradeGetElapsed(trade, timestamp);
            int secs = ms / 1000;
            int h = secs / 3600, m = (secs % 3600) / 60, s = secs % 60;
            snprintf(buffer, 128, "Trade ended: %02d:%02d:%02d | Hits: %d", h, m, s, tradeGetHits(trade));
            serverChatMessage(server, buffer);
            if (game->tradeCount < MAX_TRADES) {
                game->trades[game->tradeCount++] = *trade;
            }
            tradeClear(trade);
        } else {
            serverChatMessage(server, "No trade running");
        }
        return true;
    }

    if (strcmp(command.argv[1], "cancel") == 0) {
        if (tradeCancel(trade)) {
            serverChatMessage(server, "Trade cancelled");
        } else {
            serverChatMessage(server, "No trade running");
        }
        return true;
    }

    if (strcmp(command.argv[1], "total") == 0) {
        if (game->tradeCount == 0) {
            serverChatMessage(server, "No trades recorded");
            return true;
        }
        int totalMs = 0, totalHits = 0;
        for (int i = 0; i < game->tradeCount; i++) {
            totalMs += game->trades[i].endTimestamp - game->trades[i].startTimestamp;
            totalHits += game->trades[i].hits;
        }
        int secs = totalMs / 1000;
        int h = secs / 3600, m = (secs % 3600) / 60, s = secs % 60;
        snprintf(buffer, 128, "Total: %02d:%02d:%02d | Hits: %d (%d trades)", h, m, s, totalHits, game->tradeCount);
        serverChatMessage(server, buffer);
        return true;
    }

    serverChatMessage(server, "Usage: /trade [start | end | cancel | total]");
    return false;
}

bool commandRevivesHandle(Command command) {
    (void)command;
    State *state = controllerGetState(controller);
    Game *game = &state->activeGame;
    if (!game) return false;
    int quickRevives = gameGetQuickRevivesDrunk(game);
    char buffer[64];
    snprintf(buffer, 64, "Quick Revives Drunk: %d", quickRevives);
    serverChatMessage(server, buffer);
    return true;
}
