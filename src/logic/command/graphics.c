#include "logic/command/graphics.h"
#include "controller/controller_internal.h"
#include "logic/server.h"
#include "client.h"
#include "client/graphics.h"
#include "service.h"
#include <stdlib.h>
#include <stdio.h>

static Server *server;
static int clientPort;

void commandGraphicsInit(Controller *controllerInstance) {
    server = _controllerGetServer(controllerInstance);
    clientPort = serviceResolvePort();
}

static bool getGraphics(Client *client, GraphicsConfig *out) {
    return clientGetGraphics(client, out) == CLIENT_OK;
}

typedef enum {
    GFX_BORDERLESS,
    GFX_UNLIMIT_FPS,
    GFX_DISABLE_HUD,
    GFX_DISABLE_FOG,
    GFX_FULLBRIGHT,
    GFX_COLORIZED,
} GraphicsToggle;

static bool applyToggle(GraphicsToggle which) {
    Client *client = clientCreate(clientPort);
    if (!client) return false;

    GraphicsConfig config;
    bool ok = false;
    if (getGraphics(client, &config)) {
        switch (which) {
            case GFX_BORDERLESS:  config.borderless = !config.borderless; break;
            case GFX_UNLIMIT_FPS: config.unlimitFps = !config.unlimitFps; break;
            case GFX_DISABLE_HUD: config.disableHud = !config.disableHud; break;
            case GFX_DISABLE_FOG: config.disableFog = !config.disableFog; break;
            case GFX_FULLBRIGHT:  config.fullbright = !config.fullbright; break;
            case GFX_COLORIZED:   config.colorized  = !config.colorized;  break;
        }
        ok = clientSetGraphics(client, &config) == CLIENT_OK;
    }
    clientDestroy(client);
    return ok;
}

bool commandFovHandle(Command command) {
    char buffer[64];
    Client *client = clientCreate(clientPort);
    if (!client) return false;

    GraphicsConfig config;
    if (!getGraphics(client, &config)) {
        clientDestroy(client);
        return false;
    }

    if (command.argc == 1) {
        snprintf(buffer, 64, "FOV: %d", config.fov);
        serverChatMessage(server, buffer);
        clientDestroy(client);
        return true;
    }

    int fov = atoi(command.argv[1]);
    if (!fov) {
        serverChatMessage(server, "That's an invalid value!");
        clientDestroy(client);
        return false;
    }

    config.fov = fov;
    bool ok = clientSetGraphics(client, &config) == CLIENT_OK;
    clientDestroy(client);
    return ok;
}

bool commandFovscaleHandle(Command command) {
    char buffer[64];
    Client *client = clientCreate(clientPort);
    if (!client) return false;

    GraphicsConfig config;
    if (!getGraphics(client, &config)) {
        clientDestroy(client);
        return false;
    }

    if (command.argc == 1) {
        snprintf(buffer, 64, "FOV Scale: %d", config.fovScale);
        serverChatMessage(server, buffer);
        clientDestroy(client);
        return true;
    }

    int fovScale = atoi(command.argv[1]);
    if (fovScale <= 0) {
        serverChatMessage(server, "Invalid FOV Scale value!");
        clientDestroy(client);
        return false;
    }

    config.fovScale = fovScale;
    bool ok = clientSetGraphics(client, &config) == CLIENT_OK;
    clientDestroy(client);
    return ok;
}

bool commandFpsHandle(Command command) {
    char buffer[64];
    Client *client = clientCreate(clientPort);
    if (!client) return false;

    GraphicsConfig config;
    if (!getGraphics(client, &config)) {
        clientDestroy(client);
        return false;
    }

    if (command.argc == 1) {
        snprintf(buffer, 64, "FPS Cap: %d", config.fpsCap);
        serverChatMessage(server, buffer);
        clientDestroy(client);
        return true;
    }

    int fpsCap = atoi(command.argv[1]);
    if (fpsCap <= 0) {
        serverChatMessage(server, "Invalid FPS value!");
        clientDestroy(client);
        return false;
    }

    config.fpsCap = fpsCap;
    bool ok = clientSetGraphics(client, &config) == CLIENT_OK;
    clientDestroy(client);
    return ok;
}

bool commandBorderlessHandle(Command command) { (void)command; return applyToggle(GFX_BORDERLESS); }
bool commandUnlimitfpsHandle(Command command) { (void)command; return applyToggle(GFX_UNLIMIT_FPS); }
bool commandDisablehudHandle(Command command) { (void)command; return applyToggle(GFX_DISABLE_HUD); }
bool commandDisablefogHandle(Command command) { (void)command; return applyToggle(GFX_DISABLE_FOG); }
bool commandFullbrightHandle(Command command) { (void)command; return applyToggle(GFX_FULLBRIGHT); }
bool commandColorizedHandle(Command command) { (void)command; return applyToggle(GFX_COLORIZED); }
