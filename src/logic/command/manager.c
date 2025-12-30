#include "logic/command.h"
#include "logic/command/manager.h"
#include "logic/command/manager/manager_internal.h"
#include "logic/command/graphics.h"
#include "logic/command/hacks.h"
#include "logic/command/misc.h"
#include "controller.h"
#include "controller/controller_internal.h"
#include "utils/map.h"
#include <stdlib.h>
#include <windows.h>

static Map *createCommandsMap(void) {
    Map *map = mapCreate();

    // Hacks
    mapPutInt(map, "god", COMMAND_GOD);
    mapPutInt(map, "noclip", COMMAND_NOCLIP);
    mapPutInt(map, "invis", COMMAND_INVISIBLE);
    mapPutInt(map, "infammo", COMMAND_INFAMMO);
    mapPutInt(map, "insta", COMMAND_INSTA);
    mapPutInt(map, "norecoil", COMMAND_NORECOIL);
    mapPutInt(map, "noshellshock", COMMAND_NOSHELLSHOCK);
    mapPutInt(map, "speed", COMMAND_SPEED);
    mapPutInt(map, "knife", COMMAND_KNIFE);
    mapPutInt(map, "crosshair", COMMAND_CROSSHAIR);
    mapPutInt(map, "staticbox", COMMAND_STATICBOX);
    mapPutInt(map, "thirdperson", COMMAND_THIRDPERSON);

    // Graphics
    mapPutInt(map, "fov", COMMAND_FOV);
    mapPutInt(map, "fovscale", COMMAND_FOVSCALE);
    mapPutInt(map, "fps", COMMAND_FPS);
    mapPutInt(map, "unlimitfps", COMMAND_UNLIMITFPS);
    mapPutInt(map, "borderless", COMMAND_BORDERLESS);
    mapPutInt(map, "disablehud", COMMAND_DISABLEHUD);
    mapPutInt(map, "disablefog", COMMAND_DISABLEFOG);
    mapPutInt(map, "fullbright", COMMAND_FULLBRIGHT);
    mapPutInt(map, "colorized", COMMAND_COLORIZED);

    // GSC
    mapPutInt(map, "perk", COMMAND_PERK);

    // Misc
    mapPutInt(map, "give", COMMAND_GIVE);
    mapPutInt(map, "tp", COMMAND_TP);
    mapPutInt(map, "restart", COMMAND_RESTART);
    mapPutInt(map, "music", COMMAND_MUSIC);
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

static void initSubmodules(CommandManager *manager) {
    commandGraphicsInit(manager->controller);
    commandHacksInit(manager->controller);
    commandMiscInit(manager->controller);
}

CommandManager *commandManagerCreate(Controller *controller) {
    if (!controller) return NULL;

    CommandManager *manager = (CommandManager *)malloc(sizeof(CommandManager));
    if (!manager) return NULL;

    manager->controller = controller;
    manager->server = _controllerGetServer(controller);
    manager->commandsMap = createCommandsMap();
    manager->history = historyCreate();
    manager->prevState.chatOpen = false;
    manager->prevState.upKey = false;
    manager->prevState.downKey = false;

    initSubmodules(manager);
    return manager;
}

void commandManagerDestroy(CommandManager *manager) {
    if (!manager) return;
    
    historyDestroy(manager->history);
    mapDestroy(manager->commandsMap);
    free(manager);
}

void commandManagerAddHistory(CommandManager *manager, const Command *command) {
    if (!manager) return;
    historyAdd(manager->history, command);
}

bool commandManagerIsValid(CommandManager *manager, const char *cmd) {
    if (!manager || !cmd) return false;
    return mapContains(manager->commandsMap, cmd);
}

CommandName commandManagerGetName(CommandManager *manager, const char *cmd) {
    if (!manager || !cmd) return COMMAND_UNKNOWN;
    if (!mapContains(manager->commandsMap, cmd)) return COMMAND_UNKNOWN;
    return (CommandName)mapGetInt(manager->commandsMap, cmd);
}

void commandManagerUpdate(CommandManager *manager) {
    if (!manager || !manager->controller) return;
    if (!controllerIsGameAttached(manager->controller)) return;
    if (!controllerIsGameWindowFocused(manager->controller)) return;

    bool chatOpen = controllerIsChatOpen(manager->controller);

    // Reset history navigation when chat opens
    if (chatOpen && !manager->prevState.chatOpen) {
        historyReset(manager->history);
        manager->prevState.upKey = false;
        manager->prevState.downKey = false;
    }
    manager->prevState.chatOpen = chatOpen;
    
    if (!chatOpen) return;

    // Handle Up key
    bool upPressed = (GetAsyncKeyState(VK_UP) & 0x8000) != 0;
    if (upPressed && !manager->prevState.upKey) {
        Command *prev = historyGetPrevious(manager->history);
        if (prev) controllerWriteToChatInput(manager->controller, commandToString(prev));
    }
    manager->prevState.upKey = upPressed;

    // Handle Down key
    bool downPressed = (GetAsyncKeyState(VK_DOWN) & 0x8000) != 0;
    if (downPressed && !manager->prevState.downKey) {
        Command *next = historyGetNext(manager->history);
        controllerWriteToChatInput(manager->controller, next ? commandToString(next) : "");
    }
    manager->prevState.downKey = downPressed;
}
