#ifndef COMMAND_MISC_H_
#define COMMAND_MISC_H_

#include "logic/command.h"

typedef struct Controller Controller;

void commandMiscInit(Controller *controller);

bool commandGiveHandle(Command command);
bool commandTpHandle(Command command);
bool commandPerkHandle(Command command);
bool commandNextSpecialRoundHandle(Command command);
bool commandClaymoresHandle(Command command);
bool commandEntitiesHandle(Command command);
bool commandSphHandle(Command command);
bool commandRestartHandle(Command command);
bool commandTradeHandle(Command command);
bool commandRevivesHandle(Command command);

#endif // COMMAND_MISC_H_
