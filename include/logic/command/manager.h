#ifndef COMMAND_MANAGER_H_
#define COMMAND_MANAGER_H_

#include "logic/command.h"
#include <stdbool.h>

typedef struct CommandManager CommandManager;
typedef struct Controller Controller;

typedef struct {
    CommandName name;
    const char *usage;
    const char *description;
} CommandEntry;

typedef void (*CommandIteratorFn)(const CommandEntry *entry, void *userData);

CommandManager *commandManagerCreate(Controller *controller);
void commandManagerInitSubmodules(CommandManager *manager);
void commandManagerDestroy(CommandManager *manager);
void commandManagerAddHistory(CommandManager *manager, const Command *command);
bool commandManagerIsValid(CommandManager *manager, const char *cmd);
CommandName commandManagerGetName(CommandManager *manager, const char *cmd);
void commandManagerUpdate(CommandManager *manager);
void commandManagerForEach(CommandManager *manager, CommandIteratorFn fn, void *data);

#endif // COMMAND_MANAGER_H_
