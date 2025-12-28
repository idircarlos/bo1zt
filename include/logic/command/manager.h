#ifndef COMMAND_MANAGER_H_
#define COMMAND_MANAGER_H_

#include "logic/command.h"
#include <stdbool.h>

typedef struct CommandManager CommandManager;
typedef struct Controller Controller;

CommandManager *commandManagerCreate(Controller *controller);
void commandManagerDestroy(CommandManager *manager);
void commandManagerAddHistory(CommandManager *manager, const Command *command);
bool commandManagerIsValid(CommandManager *manager, const char *cmd);
CommandName commandManagerGetName(CommandManager *manager, const char *cmd);
void commandManagerUpdate(CommandManager *manager);

#endif // COMMAND_MANAGER_H_
