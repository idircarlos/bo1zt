#ifndef COMMAND_H_
#define COMMAND_H_

#include "../controller/controller.h"

typedef struct Command Command;

void commandInit(Controller *controller);
Command *commandPoll();
bool commandHandle(Command *command);
void commandFree(Command *command);

#endif // COMMAND_H_