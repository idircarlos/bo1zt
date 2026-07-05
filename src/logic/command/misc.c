#include "logic/command/misc.h"
#include "controller/controller_internal.h"
#include "logic/server.h"
#include "logic/game/perk.h"
#include "logic/game/weapon.h"
#include "client.h"
#include "client/player.h"
#include "client/stats.h"
#include "client/round.h"
#include "client/trade.h"
#include "client/actions.h"
#include "client/game.h"
#include "service.h"
#include "logger.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static Server *server;
static int clientPort;

void commandMiscInit(Controller *controllerInstance) {
    server = _controllerGetServer(controllerInstance);
    clientPort = serviceResolvePort();
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

static Weapon getWeaponFromAbbreviation(const char *weaponAbbreviation) {
    if (strcmp("mk", weaponAbbreviation) == 0) return WEAPON_CYMBAL_MONKEY;
    if (strcmp("bh", weaponAbbreviation) == 0) return WEAPON_BLACK_HOLE;
    if (strcmp("nd", weaponAbbreviation) == 0) return WEAPON_NESTING_DOLLS;
    if (strcmp("qb", weaponAbbreviation) == 0) return WEAPON_QUANTUM_BOMB;
    if (strcmp("ray", weaponAbbreviation) == 0) return WEAPON_RAY_GUN;
    if (strcmp("tg", weaponAbbreviation) == 0) return WEAPON_THUNDERGUN;
    if (strcmp("bow", weaponAbbreviation) == 0) return WEAPON_AWFUL_LAWTON;
    if (strcmp("mas", weaponAbbreviation) == 0) return WEAPON_MUSTANG_AND_SALLY;
    return WEAPON_INVALID;
}

static void formatDuration(int ms, char *hms, size_t size) {
    int secs = ms / 1000;
    snprintf(hms, size, "%02d:%02d:%02d", secs / 3600, (secs % 3600) / 60, secs % 60);
}

bool commandPerkHandle(Command command) {
    if (command.argc < 3) {
        serverChatMessage(server, "Usage: /perk <add | rm> <perk1> [perk2]...");
        serverChatMessage(server, "Perks: jg qr sc dt su mk");
        return false;
    }

    bool isAdd = strcmp(command.argv[1], "add") == 0;
    bool isRemove = strcmp(command.argv[1], "rm") == 0;
    if (!isAdd && !isRemove) {
        serverChatMessage(server, "Unknown perk action. Use: add, rm");
        return false;
    }

    Perk perks[16];
    int count = 0;
    for (int i = 2; i < command.argc && count < 16; i++) {
        Perk perk = getPerkFromAbbreviation(command.argv[i]);
        if (perk == PERK_INVALID) {
            LOG_WARN("Invalid perk abbreviation: %s", command.argv[i]);
            continue;
        }
        perks[count++] = perk;
    }
    if (count == 0) return false;

    Client *client = clientCreate(clientPort);
    if (!client) return false;
    ClientResult r = isAdd ? clientAddPerks(client, perks, count)
                           : clientRemovePerks(client, perks, count);
    clientDestroy(client);
    return r == CLIENT_OK;
}

bool commandGiveHandle(Command command) {
    if (command.argc < 2) {
        serverChatMessage(server, "/give must receive an argument!");
        serverChatMessage(server, "Usage: /give ammo | <weapon>");
        return false;
    }

    Client *client = clientCreate(clientPort);
    if (!client) return false;

    if (strcmp("ammo", command.argv[1]) == 0) {
        ClientResult r = clientGiveAmmo(client);
        clientDestroy(client);
        return r == CLIENT_OK;
    }

    Weapon weapons[16];
    int count = 0;
    for (int i = 1; i < command.argc && count < 16; i++) {
        Weapon weapon = getWeaponFromAbbreviation(command.argv[i]);
        if (weapon == WEAPON_INVALID) {
            LOG_WARN("Invalid weapon abbreviation: %s", command.argv[i]);
            continue;
        }
        weapons[count++] = weapon;
    }
    if (count == 0) {
        clientDestroy(client);
        return false;
    }

    ClientResult r = clientGiveWeapons(client, weapons, count);
    clientDestroy(client);
    return r == CLIENT_OK;
}

bool commandTpHandle(Command command) {
    char buffer[64];
    Client *client = clientCreate(clientPort);
    if (!client) return false;

    if (command.argc == 1) {
        TeleportCoords coords;
        if (clientGetPosition(client, &coords) == CLIENT_OK) {
            snprintf(buffer, 64, "Position: (%d, %d, %d)", (int)coords.x, (int)coords.y, (int)coords.z);
            serverChatMessage(server, buffer);
            clientDestroy(client);
            return true;
        }
        serverChatMessage(server, "Failed to get current position!");
        clientDestroy(client);
        return false;
    }

    if (command.argc < 4) {
        serverChatMessage(server, "Usage: /tp [x y z]");
        clientDestroy(client);
        return false;
    }

    float x = (float)atof(command.argv[1]);
    float y = (float)atof(command.argv[2]);
    float z = (float)atof(command.argv[3]);

    bool ok = clientTeleport(client, x, y, z) == CLIENT_OK;
    if (!ok) serverChatMessage(server, "Failed to teleport!");
    clientDestroy(client);
    return ok;
}

bool commandNextSpecialRoundHandle(Command command) {
    (void)command;
    Client *client = clientCreate(clientPort);
    if (!client) return false;

    SpecialRound special;
    ClientResult r = clientGetSpecialRound(client, &special);
    clientDestroy(client);

    if (r != CLIENT_OK) {
        serverChatMessage(server, "No special rounds");
        return false;
    }
    if (special.next[0] != '\0') {
        serverChatMessage(server, special.next);
        return true;
    }
    serverChatMessage(server, "No special rounds");
    return false;
}

bool commandClaymoresHandle(Command command) {
    (void)command;
    Client *client = clientCreate(clientPort);
    if (!client) return false;

    int claymores = 0;
    ClientResult r = clientGetClaymores(client, &claymores);
    clientDestroy(client);
    if (r != CLIENT_OK) return false;

    char buffer[64];
    snprintf(buffer, 64, "Claymores: %d", claymores);
    serverChatMessage(server, buffer);
    return true;
}

bool commandEntitiesHandle(Command command) {
    (void)command;
    Client *client = clientCreate(clientPort);
    if (!client) return false;

    int current = 0, max = 0;
    ClientResult r = clientGetEntities(client, &current, &max);
    clientDestroy(client);
    if (r != CLIENT_OK) return false;

    char buffer[64];
    snprintf(buffer, 64, "Entities: %d/%d", current, max);
    serverChatMessage(server, buffer);
    return true;
}

bool commandSphHandle(Command command) {
    char buffer[128];

    int scopeRound = 0;
    if (command.argc >= 2) {
        scopeRound = atoi(command.argv[1]);
        if (scopeRound <= 0) {
            serverChatMessage(server, "Usage: /sph or /sph <round>");
            return false;
        }
    }

    Client *client = clientCreate(clientPort);
    if (!client) return false;

    double sph = 0.0;
    ClientResult r = clientGetSph(client, scopeRound, &sph);
    clientDestroy(client);

    if (r == CLIENT_OK) {
        if (scopeRound > 0) {
            snprintf(buffer, 128, "SPH for round %d: %.1f", scopeRound, sph);
            serverChatMessage(server, buffer);
            return true;
        }
        if (sph <= 0.0) {
            serverChatMessage(server, "No valid rounds to calculate SPH yet!");
            return false;
        }
        snprintf(buffer, 128, "Average SPH: %.2f", sph);
        serverChatMessage(server, buffer);
        return true;
    }

    if (r == CLIENT_ERR_INVALID_PARAM && scopeRound > 0) {
        snprintf(buffer, 128, "Round %d was special or out of range!", scopeRound);
        serverChatMessage(server, buffer);
    } else if (r == CLIENT_ERR_CONFLICT) {
        serverChatMessage(server, "No previous rounds to display SPH for!");
    } else {
        serverChatMessage(server, "Failed to get SPH!");
    }
    return false;
}

bool commandRestartHandle(Command command) {
    (void)command;
    Client *client = clientCreate(clientPort);
    if (!client) return false;
    ClientResult r = clientRestartGame(client);
    clientDestroy(client);
    return r == CLIENT_OK;
}

bool commandMusicHandle(Command command) {
    (void)command;
    Client *client = clientCreate(clientPort);
    if (!client) return false;
    ClientResult r = clientPlayMusic(client);
    clientDestroy(client);
    return r == CLIENT_OK;
}

bool commandTradeHandle(Command command) {
    char buffer[128];
    char hms[16];
    Client *client = clientCreate(clientPort);
    if (!client) return false;

    if (command.argc < 2) {
        TradeStatus status;
        if (clientGetTrade(client, &status) != CLIENT_OK || !status.running) {
            serverChatMessage(server, "No trade running");
            clientDestroy(client);
            return true;
        }
        formatDuration(status.elapsedMs, hms, sizeof(hms));
        snprintf(buffer, 128, "Trade: %s | Hits: %d", hms, status.hits);
        serverChatMessage(server, buffer);
        clientDestroy(client);
        return true;
    }

    if (strcmp(command.argv[1], "start") == 0) {
        bool ok = clientStartTrade(client) == CLIENT_OK;
        serverChatMessage(server, ok ? "Trade started" : "Trade already running");
        clientDestroy(client);
        return true;
    }

    if (strcmp(command.argv[1], "end") == 0) {
        TradeStatus status;
        if (clientEndTrade(client, &status) == CLIENT_OK) {
            formatDuration(status.elapsedMs, hms, sizeof(hms));
            snprintf(buffer, 128, "Trade ended: %s | Hits: %d", hms, status.hits);
            serverChatMessage(server, buffer);
        } else {
            serverChatMessage(server, "No trade running");
        }
        clientDestroy(client);
        return true;
    }

    if (strcmp(command.argv[1], "cancel") == 0) {
        bool ok = clientCancelTrade(client) == CLIENT_OK;
        serverChatMessage(server, ok ? "Trade cancelled" : "No trade running");
        clientDestroy(client);
        return true;
    }

    if (strcmp(command.argv[1], "total") == 0) {
        TradeTotal total;
        if (clientGetTradeTotal(client, &total) != CLIENT_OK || total.trades == 0) {
            serverChatMessage(server, "No trades recorded");
            clientDestroy(client);
            return true;
        }
        formatDuration(total.totalMs, hms, sizeof(hms));
        snprintf(buffer, 128, "Total: %s | Hits: %d (%d trades)", hms, total.totalHits, total.trades);
        serverChatMessage(server, buffer);
        clientDestroy(client);
        return true;
    }

    serverChatMessage(server, "Usage: /trade [start | end | cancel | total]");
    clientDestroy(client);
    return false;
}

bool commandRevivesHandle(Command command) {
    (void)command;
    Client *client = clientCreate(clientPort);
    if (!client) return false;

    int revives = 0;
    ClientResult r = clientGetRevives(client, &revives);
    clientDestroy(client);
    if (r != CLIENT_OK) return false;

    char buffer[64];
    snprintf(buffer, 64, "Quick Revives Drunk: %d", revives);
    serverChatMessage(server, buffer);
    return true;
}
