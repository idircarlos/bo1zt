#include "logic/game.h"
#include "logger.h"
#include "logic/game/level.h"
#include "logic/game/round.h"
#include "logic/game/trade.h"
#include "widget/cycle.h"
#include "gui/widgets.h"
#include <stdio.h>
#include <stdlib.h>

#define max(a,b)    (((a) > (b)) ? (a) : (b))

const char* _gameGetNextPotentialDogsRounds(Game *game);
const char* _gameGetNextPotentialMonkeysRounds(Game *game);
const char* _gameGetNextPotentialThiefRounds(Game *game);

void gameInit(Game *game, int players) {
    game->levelName = LEVEL_INVALID;
    game->players = players;
    gameClear(game);
}

bool gameStart(Game *game, Level level, int startTimestamp) {
    if (!game) {
        LOG_ERROR("Couldn't start game because Game is an invalid object\n");
        return NULL;
    }
    game->levelName = level;
    game->startTimestamp = startTimestamp;
    return true;
}

bool gameStarted(Game *game) {
    return game->startTimestamp != 0;
}

bool gameEnded(Game *game) {
    return game->endTimestamp != 0;
}

bool gameRunning(Game *game) {
    return gameStarted(game) && !gameEnded(game);
}

bool gameClear(Game *game) {
    game->elapsed = 0;
    game->startTimestamp = 0;
    game->endTimestamp = 0;
    game->powerOnTimestamp = 0;
    game->powerOnRound = 0;
    game->numPerks = 0;
    game->lastPerkAcquiredOnRound = 0;
    game->totalZombies = 0;
    game->quickRevivesDrunk = 0;
    game->movementSpeed = 0;
    game->drops = 0;
    game->currentEntities = 0;
    game->maxEntities = 0;
    roundClear(&game->currentRound);
    for (int i = 0; i < MAX_ROUNDS; i++) {
        roundClear(&game->rounds[i]);
    }
    tradeClear(&game->currentTrade);
    for (int i = 0; i < MAX_TRADES; i++) {
        tradeClear(&game->trades[i]);
    }
    game->tradeCount = 0;
    return true;
}

bool gameEnd(Game *game, int endTimestamp) {
    game->endTimestamp = endTimestamp;
    return true;
}

bool gameUpdateElapsed(Game *game, int levelElapsed) {
    game->elapsed = levelElapsed - game->startTimestamp;
    Round *round = &game->currentRound;
    if (roundRunning(round)) {
        roundUpdateElapsed(round, levelElapsed);
    }
    return true;
}

bool gameRoundStarted(Game *game, int startTimestamp, bool special) {
    int nextNumber = game->currentRound.number + 1;
    roundInit(&game->currentRound, nextNumber, game->players);
    roundStart(&game->currentRound, startTimestamp, special);
    game->rounds[game->currentRound.number - 1] = game->currentRound;
    return true;
}

bool gameRoundEnded(Game *game, int endTimestamp) {
    roundEnd(&game->currentRound, endTimestamp);
    game->rounds[game->currentRound.number - 1] = game->currentRound;
    return true;
}

bool gameZombieKilled(Game *game) {
    game->totalZombies++;
    roundZombieKilled(&game->currentRound);
    return true;
}

bool gamePowerupDropped(Game *game, Powerup powerup) {
    game->drops++;
    roundPowerupDropped(&game->currentRound);
    Widget *cycle = uiWidgetsGetCycleWidget();
    cycleWidgetActivate(cycle, powerup);
    return true;
}

bool gamePowerupNewCycle(Game *game) {
    (void)game;
    Widget *cycle = uiWidgetsGetCycleWidget();
    cycleWidgetReset(cycle);
    return true;
}

bool gamePowerOn(Game *game, int timestamp) {
    game->powerOnTimestamp = timestamp;
    game->powerOnRound = game->currentRound.number;
    return true;
}

bool gameSetNumPerks(Game *game, int numPerks) {
    int currentNumPerks = game->numPerks;
    if (numPerks > currentNumPerks) {
        Round currentRound = game->currentRound;
        game->lastPerkAcquiredOnRound = currentRound.number;
    }
    game->numPerks = numPerks;
    return true;
}

bool gameSetQuickRevivesDrunk(Game *game, int quickRevivesDrunk) {
    game->quickRevivesDrunk = quickRevivesDrunk;
    return true;
}

int gameGetQuickRevivesDrunk(Game *game) {
    return game->quickRevivesDrunk;
}

bool gamePerkAcquired(Game *game) {
    Round currentRound = game->currentRound;
    game->lastPerkAcquiredOnRound = currentRound.number;
    game->numPerks++;
    return true;
}

bool gamePerkLost(Game *game) {
    game->numPerks--;
    return true;
}

const char* gameNextPotentialSpecialRounds(Game *game) {
    RoundType specialRound = levelGetSpecialRound(game->levelName);
    switch (specialRound) {
        case RT_DOGS: return _gameGetNextPotentialDogsRounds(game);
        case RT_MONKEYS: return _gameGetNextPotentialMonkeysRounds(game);
        case RT_THIEF: return _gameGetNextPotentialThiefRounds(game);
        default: return "Unimplemented";
    }
}

bool gameTradeHit(Game *game) {
    return tradeHit(&game->currentTrade);
}

void gamePrint(Game *game) {
    LOG_INFO("Game { elapsed: %d, start: %d, end: %d, players: %d, qr: %d, zombies: %d, drops: %d, level: %d }\n",
        game->elapsed, game->startTimestamp, game->endTimestamp, game->players,
        game->quickRevivesDrunk, game->totalZombies, game->drops, game->levelName);
}

static int _gameGetLastSpecialRound(Game *game) {
    for (int i = game->currentRound.number - 1; i >= 0; i--) {
        if (game->rounds[i].isSpecial) {
            return game->rounds[i].number;
        }
    }
    return 0;
}

const char *_gameGetNextPotentialDogsRounds(Game *game) {
    if (!game || game->startTimestamp == 0) {
        return NULL;
    }

    if (levelGetSpecialRound(game->levelName) != RT_DOGS) {
        return NULL;
    }

    char *result = (char*)malloc(32);
    if (!result) return NULL;
    
    int offset = 0;
    int lastSpecialRound = _gameGetLastSpecialRound(game);

    if (lastSpecialRound == 0) {
        int start = (game->currentRound.number >= 5) ? (game->currentRound.number + 1) : 5;
        for (int r = start; r <= 7; r++) {
            offset += snprintf(result + offset, 32 - offset, "%s%d", offset ? ", " : "", r);
        }
    } else {
        for (int r = lastSpecialRound + 4; r < lastSpecialRound + 6; r++) {
            if (r > game->currentRound.number) {
                offset += snprintf(result + offset, 32 - offset, "%s%d", offset ? ", " : "", r);
            }
        }
    }

    if (offset == 0) {
        free(result);
        return NULL;
    }

    return result;
}

const char *_gameGetNextPotentialMonkeysRounds(Game *game) {
    if (!game || game->startTimestamp == 0) {
        return NULL;
    }

    if (levelGetSpecialRound(game->levelName) != RT_MONKEYS) {
        return NULL;
    }

    if (game->powerOnTimestamp == 0) {
        return "Power must be turned on";
    }

    if (game->numPerks == 0 || game->lastPerkAcquiredOnRound == 0) {
        return "You must have at least one perk";
    }

    char *result = (char*)malloc(64);
    if (!result) return NULL;

    int offset = 0;
    int lastSpecialRound = _gameGetLastSpecialRound(game);

    if (lastSpecialRound == 0) {
        int firstRound = max(game->currentRound.number + 1, game->lastPerkAcquiredOnRound + 1);
        for (int r = firstRound; r <= game->lastPerkAcquiredOnRound + 4; r++) {
            offset += snprintf(result + offset, 64 - offset, "%s%d", offset ? ", " : "", r);
        }
    } else if (game->currentRound.number - lastSpecialRound >= 5) {
        snprintf(result, 64, "The round after you buy a perk again.");
        return result;
    } else {
        for (int r = lastSpecialRound + 4; r < lastSpecialRound + 6; r++) {
            if (r > game->currentRound.number) {
                offset += snprintf(result + offset, 64 - offset, "%s%d", offset ? ", " : "", r);
            }
        }
    }

    if (offset == 0) {
        free(result);
        return NULL;
    }

    return result;
}

const char *_gameGetNextPotentialThiefRounds(Game *game) {
    if (!game || game->startTimestamp == 0) {
        return NULL;
    }

    if (levelGetSpecialRound(game->levelName) != RT_THIEF) {
        return NULL;
    }

    if (game->powerOnTimestamp == 0) {
        return NULL;
    }

    char *result = (char*)malloc(32);
    if (!result) return NULL;

    int offset = 0;
    int lastSpecialRound = _gameGetLastSpecialRound(game);

    if (lastSpecialRound == 0) {
        for (int r = game->powerOnRound + 1; r <= game->powerOnRound + 4; r++) {
            offset += snprintf(result + offset, 32 - offset, "%s%d", offset ? ", " : "", r);
        }
    } else {
        for (int r = lastSpecialRound + 4; r < lastSpecialRound + 6; r++) {
            if (r > game->currentRound.number) {
                offset += snprintf(result + offset, 32 - offset, "%s%d", offset ? ", " : "", r);
            }
        }
    }

    if (offset == 0) {
        free(result);
        return NULL;
    }

    return result;
}
