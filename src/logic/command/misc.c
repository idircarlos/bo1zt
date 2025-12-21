#include "logic/command/misc.h"
#include "logic/cheat.h"
#include "logic/game/perk.h"
#include "logger.h"
#include "utils/list.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static Server *server;
static Controller *controller;
static Api *api;

void commandMiscInit(Server *serverInstance, Controller *controllerInstance, Api *apiInstance) {
    server = serverInstance;
    controller = controllerInstance;
    api = apiInstance;
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

    if (success) {
        snprintf(buffer, 64, "Teleported to (%d, %d, %d)", (int)x, (int)y, (int)z);
        serverChatMessage(server, buffer);
    } else {
        serverChatMessage(server, "Failed to teleport!");
    }

    return success;
}
