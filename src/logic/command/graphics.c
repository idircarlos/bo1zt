#include "logic/command/graphics.h"
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

    config->graphics.fov = fov;
    configSave(config);
    snprintf(buffer, 64, "FOV set to %d", fov);
    serverChatMessage(server, buffer);
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

    config->graphics.fovScale = fovScale;
    configSave(config);
    snprintf(buffer, 64, "FOV Scale set to %d", fovScale);
    serverChatMessage(server, buffer);
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

    config->graphics.fpsCap = fpsCap;
    configSave(config);
    snprintf(buffer, 64, "FPS Cap set to %d", fpsCap);
    serverChatMessage(server, buffer);
    return true;
}

bool commandBorderlessHandle(Command command) { (void)command; return toggle(CHEAT_NAME_MAKE_BORDERLESS); }
bool commandUnlimitfpsHandle(Command command) { (void)command; return toggle(CHEAT_NAME_UNLIMIT_FPS); }
bool commandDisablehudHandle(Command command) { (void)command; return toggle(CHEAT_NAME_DISABLE_HUD); }
bool commandDisablefogHandle(Command command) { (void)command; return toggle(CHEAT_NAME_DISABLE_FOG); }
bool commandFullbrightHandle(Command command) { (void)command; return toggle(CHEAT_NAME_FULLBRIGHT); }
bool commandColorizedHandle(Command command) { (void)command; return toggle(CHEAT_NAME_COLORIZED); }
