#ifndef COMMAND_MISC_H_
#define COMMAND_MISC_H_

#include "logic/command.h"
#include "logic/server.h"
#include "api.h"

void commandMiscInit(Server *server, Controller *controller, Api *api);

bool commandGiveHandle(Command command);
bool commandTpHandle(Command command);
bool commandPerkHandle(Command command);
bool commandNextSpecialRoundHandle(Command command);

#endif // COMMAND_MISC_H_
