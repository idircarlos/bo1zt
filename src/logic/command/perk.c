#include "logic/command/perk.h"
#include "api/gsc.h"
#include "logger/logger.h"
#include "utils/list.h"
#include <string.h>

static Server *server;
static ApiGsc *apiGsc;

Perk commandPerkGetFromAbbreviation(const char *perkAbbreviation) {
    if (strcmp("qr", perkAbbreviation) == 0) return PERK_QUICK_REVIVE;
    if (strcmp("jg", perkAbbreviation) == 0) return PERK_JUGGERNAUT;
    if (strcmp("sc", perkAbbreviation) == 0) return PERK_SPEED_COLA;
    if (strcmp("dt", perkAbbreviation) == 0) return PERK_DOUBLE_TAP;
    if (strcmp("su", perkAbbreviation) == 0) return PERK_STAMINA_UP;
    if (strcmp("mk", perkAbbreviation) == 0) return PERK_MULE_KICK;
    return PERK_INVALID;
}

void commandPerkInit(Server *serverInstance, ApiGsc *apiGscInstance) {
    server = serverInstance;
    apiGsc = apiGscInstance;
}

static List *buildPerkList(Command command) {
    List *perks = listCreate();

    for (int i = 2; i < command.argc; i++) {
        Perk perk = commandPerkGetFromAbbreviation(command.argv[i]);
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
        serverChatMessage(server, "Usage: /perk <add | rm> <perk1> [perk2] ...");
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
        success = apiGscAddPerks(apiGsc, perks);
    } else if (strcmp(command.argv[1], "rm") == 0) {
        success = apiGscRemovePerks(apiGsc, perks);
    } else {
        serverChatMessage(server, "Unknown perk action. Use: add, rm");
    }

    listDestroy(perks);
    return success;
}
