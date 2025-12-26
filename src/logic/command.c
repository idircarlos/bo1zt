#include "logic/command.h"
#include "controller.h"
#include "logic/cheat.h"
#include "logic/command/graphics.h"
#include "logic/command/hacks.h"
#include "logic/command/misc.h"
#include "logic/server.h"
#include "controller/controller_internal.h"
#include "utils/map.h"
#include <stdlib.h>
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
    // Hacks
    mapPutInt(map, "infammo", COMMAND_INFAMMO);
    mapPutInt(map, "insta", COMMAND_INSTA);
    mapPutInt(map, "god", COMMAND_GOD);
    mapPutInt(map, "noclip", COMMAND_NOCLIP);
    mapPutInt(map, "invis", COMMAND_INVISIBLE);
    mapPutInt(map, "norecoil", COMMAND_NORECOIL);
    mapPutInt(map, "staticbox", COMMAND_STATICBOX);
    mapPutInt(map, "thirdperson", COMMAND_THIRDPERSON);
    mapPutInt(map, "crosshair", COMMAND_CROSSHAIR);
    mapPutInt(map, "speed", COMMAND_SPEED);
    mapPutInt(map, "noshellshock", COMMAND_NOSHELLSHOCK);
    mapPutInt(map, "knife", COMMAND_KNIFE);
    
    // Graphics
    mapPutInt(map, "fov", COMMAND_FOV);
    mapPutInt(map, "fovscale", COMMAND_FOVSCALE);
    mapPutInt(map, "fps", COMMAND_FPS);
    mapPutInt(map, "borderless", COMMAND_BORDERLESS);
    mapPutInt(map, "unlimitfps", COMMAND_UNLIMITFPS);
    mapPutInt(map, "disablehud", COMMAND_DISABLEHUD);
    mapPutInt(map, "disablefog", COMMAND_DISABLEFOG);
    mapPutInt(map, "fullbright", COMMAND_FULLBRIGHT);
    mapPutInt(map, "colorized", COMMAND_COLORIZED);

    // GSC
    mapPutInt(map, "perk", COMMAND_PERK);
    
    // Others
    mapPutInt(map, "give", COMMAND_GIVE);
    mapPutInt(map, "tp", COMMAND_TP);
    mapPutInt(map, "restart", COMMAND_RESTART);
    mapPutInt(map, "uwu", COMMAND_UWU);

    // Special rounds
    mapPutInt(map, "dogs", COMMAND_DOGS);
    mapPutInt(map, "monkeys", COMMAND_MONKEYS);
    mapPutInt(map, "thief", COMMAND_THIEF);

    // Info
    mapPutInt(map, "claymores", COMMAND_CLAYMORES);
    mapPutInt(map, "entities", COMMAND_ENTITIES);
    mapPutInt(map, "sph", COMMAND_SPH);
    mapPutInt(map, "trade", COMMAND_TRADE);
    mapPutInt(map, "revives", COMMAND_REVIVES);
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
    commandGraphicsInit(server, controller);
    commandHacksInit(controller);
    commandMiscInit(server, controller, _controllerGetApi(controller));
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
    switch (command.name) {
        case COMMAND_NOCLIP:
            return commandNoclipHandle(command);
        case COMMAND_GOD:
            return commandGodHandle(command);
        case COMMAND_INVISIBLE:
            return commandInvisibleHandle(command);
        case COMMAND_GIVE:
            return commandGiveHandle(command);
        case COMMAND_FOV:
            return commandFovHandle(command);
        case COMMAND_FOVSCALE:
            return commandFovscaleHandle(command);
        case COMMAND_FPS:
            return commandFpsHandle(command);
        case COMMAND_BORDERLESS:
            return commandBorderlessHandle(command);
        case COMMAND_UNLIMITFPS:
            return commandUnlimitfpsHandle(command);
        case COMMAND_DISABLEHUD:
            return commandDisablehudHandle(command);
        case COMMAND_DISABLEFOG:
            return commandDisablefogHandle(command);
        case COMMAND_FULLBRIGHT:
            return commandFullbrightHandle(command);
        case COMMAND_COLORIZED:
            return commandColorizedHandle(command);
        case COMMAND_INSTA:
            return commandInstaHandle(command);
        case COMMAND_INFAMMO:
            return commandInfammoHandle(command);
        case COMMAND_TP:
            return commandTpHandle(command);
        case COMMAND_PERK:
            return commandPerkHandle(command);
        case COMMAND_NORECOIL:
            return commandNorecoilHandle(command);
        case COMMAND_CROSSHAIR:
            return commandCrosshairHandle(command);
        case COMMAND_SPEED:
            return commandSpeedHandle(command);
        case COMMAND_NOSHELLSHOCK:
            return commandNoshellshockHandle(command);
        case COMMAND_KNIFE:
            return commandKnifeHandle(command);
        case COMMAND_STATICBOX:
            return commandStaticboxHandle(command);
        case COMMAND_THIRDPERSON:
            return commandThirdpersonHandle(command);
        case COMMAND_UWU:
            return serverCenterMessage(server, "UwU :3");
        case COMMAND_DOGS:
        case COMMAND_MONKEYS:
        case COMMAND_THIEF:
            return commandNextSpecialRoundHandle(command);
        case COMMAND_CLAYMORES:
            return commandClaymoresHandle(command);
        case COMMAND_ENTITIES:
            return commandEntitiesHandle(command);
        case COMMAND_SPH:
            return commandSphHandle(command);
        case COMMAND_RESTART:
            return commandRestartHandle(command);
        case COMMAND_TRADE:
            return commandTradeHandle(command);
        case COMMAND_REVIVES:
            return commandRevivesHandle(command);
        default:
            return false;
    }
}
