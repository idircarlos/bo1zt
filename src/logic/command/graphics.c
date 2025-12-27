#include "logic/command/graphics.h"
#include "logic/cheat.h"
#include "logic/cheat/manager.h"
#include "logic/cheat/manager/actions.h"
#include "logic/config.h"
#include <stdlib.h>
#include <stdio.h>

static Server *server;
static Controller *controller;

void commandGraphicsInit(Server *serverInstance, Controller *controllerInstance) {
    server = serverInstance;
    controller = controllerInstance;
}

static bool toggle(CheatName cheat) {
    return cheatManagerToggle(controllerGetCheatManager(controller), cheat) != CHEAT_RESULT_API_FAILED;
}

static bool setValue(SimpleCheatName cheat, void *value) {
    return cheatManagerSetValue(controllerGetCheatManager(controller), cheat, value) != CHEAT_RESULT_API_FAILED;
}

bool commandFovHandle(Command command) {
    char buffer[64];
    Config *config = controllerGetConfig(controller);

    if (command.argc == 1) {
        snprintf(buffer, 64, "FOV: %d", config->graphics.fov);
        serverChatMessage(server, buffer);
        return true;
    }

    int fov = atoi(command.argv[1]);
    if (!fov) {
        serverChatMessage(server, "That's an invalid value!");
        return false;
    }

    setValue(SIMPLE_CHEAT_NAME_FOV, &fov);
    return true;
}

bool commandFovscaleHandle(Command command) {
    char buffer[64];
    Config *config = controllerGetConfig(controller);

    if (command.argc == 1) {
        snprintf(buffer, 64, "FOV Scale: %d", config->graphics.fovScale);
        serverChatMessage(server, buffer);
        return true;
    }

    int fovScale = atoi(command.argv[1]);
    if (fovScale <= 0) {
        serverChatMessage(server, "Invalid FOV Scale value!");
        return false;
    }

    setValue(SIMPLE_CHEAT_NAME_FOV_SCALE, &fovScale);
    return true;
}

bool commandFpsHandle(Command command) {
    char buffer[64];
    Config *config = controllerGetConfig(controller);

    if (command.argc == 1) {
        snprintf(buffer, 64, "FPS Cap: %d", config->graphics.fpsCap);
        serverChatMessage(server, buffer);
        return true;
    }

    int fpsCap = atoi(command.argv[1]);
    if (fpsCap <= 0) {
        serverChatMessage(server, "Invalid FPS value!");
        return false;
    }

    setValue(SIMPLE_CHEAT_NAME_FPS_CAP, &fpsCap);
    return true;
}

bool commandBorderlessHandle(Command command) { (void)command; return toggle(CHEAT_NAME_MAKE_BORDERLESS); }
bool commandUnlimitfpsHandle(Command command) { (void)command; return toggle(CHEAT_NAME_UNLIMIT_FPS); }
bool commandDisablehudHandle(Command command) { (void)command; return toggle(CHEAT_NAME_DISABLE_HUD); }
bool commandDisablefogHandle(Command command) { (void)command; return toggle(CHEAT_NAME_DISABLE_FOG); }
bool commandFullbrightHandle(Command command) { (void)command; return toggle(CHEAT_NAME_FULLBRIGHT); }
bool commandColorizedHandle(Command command) { (void)command; return toggle(CHEAT_NAME_COLORIZED); }
