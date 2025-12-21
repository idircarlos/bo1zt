#ifndef COMMAND_HACKS_H_
#define COMMAND_HACKS_H_

#include "logic/command.h"

// Forward declaration for API compatibility
typedef struct Server Server;

void commandHacksInit(Controller *controller);

bool commandNoclipHandle(Command command);
bool commandGodHandle(Command command);
bool commandInvisibleHandle(Command command);
bool commandInfammoHandle(Command command);
bool commandInstaHandle(Command command);
bool commandNorecoilHandle(Command command);
bool commandCrosshairHandle(Command command);
bool commandSpeedHandle(Command command);
bool commandNoshellshockHandle(Command command);
bool commandKnifeHandle(Command command);
bool commandStaticboxHandle(Command command);
bool commandThirdpersonHandle(Command command);

#endif // COMMAND_HACKS_H_
