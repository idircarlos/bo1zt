#include "logic/command.h"
#include "logic/server.h"
#include "controller/controller_internal.h"
#include "utils/map.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ARGV_DEFAULT_CAP 2
#define COMMAND_PREFIX '/'

static Controller *controller;
static Server *server;
static Map *commandsMap;

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

static bool isCommand(const char *message) {
    return message[0] == COMMAND_PREFIX;
}

static Map* createCommandsMap() {
    Map *map = mapCreate();
    mapPutInt(map, "noclip", COMMAND_NOCLIP);
    mapPutInt(map, "god", COMMAND_GOD);
    mapPutInt(map, "demigod", COMMAND_DEMIGOD);
    mapPutInt(map, "invisible", COMMAND_INVISIBLE);
    mapPutInt(map, "ufo", COMMAND_UFO);
    mapPutInt(map, "give", COMMAND_GIVE);
    mapPutInt(map, "fov", COMMAND_FOV);
    mapPutInt(map, "fovscale", COMMAND_FOVSCALE);
    mapPutInt(map, "fps", COMMAND_FPS);
    mapPutInt(map, "ndogs", COMMAND_NDOGS);
    mapPutInt(map, "insta", COMMAND_INSTA);
    mapPutInt(map, "infammo", COMMAND_INFAMMO);
    mapPutInt(map, "tp", COMMAND_TP);
    mapPutInt(map, "uwu", COMMAND_UWU);
    return map;
}

Command commandCreate(CommandName name, int argc, char **argv) {
    Command command;
    command.name = name;
    command.argc = argc;
    command.argv = argv;
    return command;
}

void commandInit(Controller *controllerInstance) {
    controller = controllerInstance;
    server = _controllerGetServer(controller);
    commandsMap = createCommandsMap();
}

Command commandBuild(const char *message) {
    int argc;
    char **argv;
    Command command = commandCreate(COMMAND_NONE, 0, NULL);
    if (message == NULL || strcmp(message, "") == 0 || !isCommand(message)) return command;
    
    commandParseMessage(message, &argc, &argv);
    char *cmdWithoutPrefix = strdup(argv[0] + 1); // Remove the '/' prefix
    free(argv[0]);
    argv[0] = cmdWithoutPrefix;

    if (!mapContains(commandsMap, argv[0])) {
        command = commandCreate(COMMAND_UNKNOWN, argc, argv);
        return command;
    }

    CommandName name = (CommandName)mapGetInt(commandsMap, argv[0]);
    command = commandCreate(name, argc, argv);
    return command;
}

bool commandHandle(Command command) {
    char buffer[64];
    switch (command.name) {
        case COMMAND_NOCLIP:
        case COMMAND_GOD:
        case COMMAND_DEMIGOD:
        case COMMAND_UFO:
            serverExecuteCommand(server, command.argv[0]);
            break;
        case COMMAND_INVISIBLE:
            serverExecuteCommand(server, "notarget");
            break;
        case COMMAND_GIVE:
            if (command.argc < 2) {
                serverChatMessage(server, "/give must receive an argument! Usage:\n/give ammo\n/give <weapon>");
                return false;
            }
            // This method specifically for some reason writes the arguments in the same address
            // where the game reads the last chat message. This is not a problem since it will be detected
            // as a usual chat message next time, so it will be ignored, but something to keep in mind.
            snprintf(buffer, 64, "give %s", command.argv[1]);
            serverExecuteCommand(server, buffer);
            break;
        case COMMAND_FOV:
            // TODO: Implement rest of commands
        default:
            return false;
    }
    return true;
}
