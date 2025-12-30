#include "logic/command.h"
#include "logic/command/manager.h"
#include "logic/command/manager/manager_internal.h"
#include "logic/command/hacks.h"
#include "logic/command/graphics.h"
#include "logic/command/misc.h"
#include "win/thread.h"
#include <stdlib.h>
#include <string.h>

#define ARGV_DEFAULT_CAP 2
#define COMMAND_PREFIX '/'

static void commandParseMessage(const char *message, int *argc, char ***argv) {
    char *msg_copy = strdup(message);
    char *token;
    int count = 0;
    int capacity = ARGV_DEFAULT_CAP;

    char **args = (char**)malloc(capacity * sizeof(char*));
    token = strtok(msg_copy, " ");
    while (token != NULL) {
        if (count >= capacity) {
            capacity *= 2;
            args = (char**)realloc(args, capacity * sizeof(char*));
        }
        args[count++] = strdup(token);
        token = strtok(NULL, " ");
    }

    *argc = count;
    *argv = args;
    free(msg_copy);
}

Command commandCreate(CommandName name, int argc, char **argv) {
    Command c;
    c.name = name;
    c.argc = argc;
    c.argv = argv;
    return c;
}

Command *commandCopy(const Command *command) {
    if (!command) return NULL;
    
    Command *copy = (Command *)malloc(sizeof(Command));
    if (!copy) return NULL;
    
    copy->name = command->name;
    copy->argc = command->argc;
    copy->argv = NULL;
    
    if (command->argc > 0 && command->argv) {
        copy->argv = (char **)malloc(command->argc * sizeof(char *));
        if (!copy->argv) {
            free(copy);
            return NULL;
        }
        for (int i = 0; i < command->argc; i++) {
            copy->argv[i] = command->argv[i] ? strdup(command->argv[i]) : NULL;
        }
    }
    
    return copy;
}

void commandFree(Command *command) {
    if (!command) return;
    
    if (command->argv) {
        for (int i = 0; i < command->argc; i++) {
            free(command->argv[i]);
        }
        free(command->argv);
    }
    free(command);
}

Command commandBuild(CommandManager *manager, const char *message) {
    if (!manager) return commandCreate(COMMAND_NONE, 0, NULL);
    if (!message || message[0] == '\0' || message[0] != COMMAND_PREFIX) {
        return commandCreate(COMMAND_NONE, 0, NULL);
    }

    int argc;
    char **argv;
    commandParseMessage(message, &argc, &argv);

    char *cmdWithoutPrefix = strdup(argv[0] + 1);
    free(argv[0]);
    argv[0] = cmdWithoutPrefix;

    if (!commandManagerIsValid(manager, argv[0])) {
        return commandCreate(COMMAND_UNKNOWN, argc, argv);
    }

    CommandName name = commandManagerGetName(manager, argv[0]);
    Command command = commandCreate(name, argc, argv);
    commandManagerAddHistory(manager, &command);
    return command;
}


int _commandThreadHandler(void *data) {
    if (!data) return 0;
    Command *commandPtr = (Command*)data;
    Command command = *commandPtr;
    switch (command.name) {
        case COMMAND_NOCLIP: return commandNoclipHandle(command);
        case COMMAND_GOD: return commandGodHandle(command);
        case COMMAND_INVISIBLE: return commandInvisibleHandle(command);
        case COMMAND_GIVE: return commandGiveHandle(command);
        case COMMAND_FOV: return commandFovHandle(command);
        case COMMAND_FOVSCALE: return commandFovscaleHandle(command);
        case COMMAND_FPS: return commandFpsHandle(command);
        case COMMAND_BORDERLESS: return commandBorderlessHandle(command);
        case COMMAND_UNLIMITFPS: return commandUnlimitfpsHandle(command);
        case COMMAND_DISABLEHUD: return commandDisablehudHandle(command);
        case COMMAND_DISABLEFOG: return commandDisablefogHandle(command);
        case COMMAND_FULLBRIGHT: return commandFullbrightHandle(command);
        case COMMAND_COLORIZED: return commandColorizedHandle(command);
        case COMMAND_INSTA: return commandInstaHandle(command);
        case COMMAND_INFAMMO: return commandInfammoHandle(command);
        case COMMAND_TP: return commandTpHandle(command);
        case COMMAND_PERK: return commandPerkHandle(command);
        case COMMAND_NORECOIL: return commandNorecoilHandle(command);
        case COMMAND_CROSSHAIR: return commandCrosshairHandle(command);
        case COMMAND_SPEED: return commandSpeedHandle(command);
        case COMMAND_NOSHELLSHOCK: return commandNoshellshockHandle(command);
        case COMMAND_KNIFE: return commandKnifeHandle(command);
        case COMMAND_STATICBOX: return commandStaticboxHandle(command);
        case COMMAND_THIRDPERSON: return commandThirdpersonHandle(command);
        case COMMAND_UWU: return false;
        case COMMAND_DOGS:
        case COMMAND_MONKEYS:
        case COMMAND_THIEF: return commandNextSpecialRoundHandle(command);
        case COMMAND_CLAYMORES: return commandClaymoresHandle(command);
        case COMMAND_ENTITIES: return commandEntitiesHandle(command);
        case COMMAND_SPH: return commandSphHandle(command);
        case COMMAND_RESTART: return commandRestartHandle(command);
        case COMMAND_MUSIC: return commandMusicHandle(command);
        case COMMAND_TRADE: return commandTradeHandle(command);
        case COMMAND_REVIVES: return commandRevivesHandle(command);
        default: return false;
    }
    commandFree(commandPtr);
    return 1;
}

int _commandThreadHandlerOnError(void *data) {
    if (!data) return 1;
    Command *command = (Command*)data;
    free(command);
    return 1;
}

bool commandHandle(Command command) {
    Command *commandCpy = commandCopy(&command);
    Thread *commandThread = threadCreate(_commandThreadHandler, commandCpy);
    threadCreateWatchdog(commandThread, 3000,  _commandThreadHandlerOnError, commandCpy);
    return true;
}

const char *commandToString(const Command *command) {
    if (!command || command->argc <= 0 || !command->argv) {
        return NULL;
    }

    size_t len = 1;
    for (int i = 0; i < command->argc; i++) {
        len += strlen(command->argv[i]);
        if (i < command->argc - 1)
            len += 1;
    }
    len += 1;

    char *result = (char *)malloc(len);
    if (!result) return NULL;

    result[0] = '/';
    result[1] = '\0';

    for (int i = 0; i < command->argc; i++) {
        strcat(result, command->argv[i]);
        if (i < command->argc - 1)
            strcat(result, " ");
    }

    return result;
}
