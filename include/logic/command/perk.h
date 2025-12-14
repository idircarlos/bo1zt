#ifndef COMMAND_PERK_H_
#define COMMAND_PERK_H_

#include "logic/command.h"
#include "logic/game/perk.h"
#include "logic/server.h"
#include "api/gsc.h"
#include <stdbool.h>

Perk commandPerkGetFromAbbreviation(const char *perkAbbreviation);
void commandPerkInit(Server *serverInstance, ApiGsc *apiGscInstance);
bool commandPerkHandle(Command command);

#endif // COMMAND_PERK_H_
