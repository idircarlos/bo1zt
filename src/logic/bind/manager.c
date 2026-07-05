#include "logic/bind/manager.h"
#include "logic/bind/keymap.h"
#include "logic/command.h"
#include "logic/command/manager.h"
#include "logic/config.h"
#include "controller.h"
#include "logger.h"
#include <windows.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    char keyName[MAX_KEY_NAME_LENGTH];
    char command[MAX_COMMAND_LENGTH];
    int vkCode;
} Bind;

struct BindManager {
    Controller *controller;
    CommandManager *commandManager;
    Bind binds[MAX_BINDS];
    int bindCount;
    bool prevKeyStates[256];
};

static inline bool isValidVKCode(int vkCode) {
    return vkCode > 0 && vkCode < 256;
}

static void executeCommands(BindManager *manager, const char *commandString) {
    if (commandString == NULL || commandString[0] == '\0') return;

    const char *ptr = commandString;
    while (*ptr == ' ' || *ptr == '\t') ptr++;

    while (*ptr != '\0') {
        if (*ptr != '/') {
            ptr++;
            continue;
        }
        const char *cmdStart = ptr;
        ptr++; // skip the leading '/'
        while (*ptr != '\0' && *ptr != '/') ptr++;

        size_t cmdLen = (size_t)(ptr - cmdStart);
        char *cmd = (char *)malloc(cmdLen + 1);
        if (!cmd) return;
        strncpy(cmd, cmdStart, cmdLen);
        cmd[cmdLen] = '\0';

        // Trim trailing whitespace.
        char *end = cmd + strlen(cmd) - 1;
        while (end > cmd && (*end == ' ' || *end == '\t')) {
            *end = '\0';
            end--;
        }

        if (cmd[0] != '\0') {
            Command builtCmd = commandBuild(manager->commandManager, cmd);
            if (builtCmd.name != COMMAND_NONE && builtCmd.name != COMMAND_UNKNOWN) {
                commandHandle(builtCmd);
            }
        }
        free(cmd);
    }
}

static void loadBinds(BindManager *manager) {
    BindsConfig cfg = controllerGetBindsConfig(manager->controller);
    manager->bindCount = 0;
    for (int i = 0; i < cfg.bindCount && manager->bindCount < MAX_BINDS; i++) {
        if (cfg.binds[i].keyName[0] == '\0' || cfg.binds[i].command[0] == '\0') continue;
        Bind *b = &manager->binds[manager->bindCount];
        strncpy(b->keyName, cfg.binds[i].keyName, MAX_KEY_NAME_LENGTH - 1);
        b->keyName[MAX_KEY_NAME_LENGTH - 1] = '\0';
        strncpy(b->command, cfg.binds[i].command, MAX_COMMAND_LENGTH - 1);
        b->command[MAX_COMMAND_LENGTH - 1] = '\0';
        b->vkCode = keymapGetVKCode(b->keyName);
        manager->bindCount++;
    }
    // Seed edge detector with the current key states so a key already held when
    // (re)loading doesn't trigger immediately.
    for (int i = 0; i < manager->bindCount; i++) {
        int vk = manager->binds[i].vkCode;
        if (isValidVKCode(vk)) {
            manager->prevKeyStates[vk] = (GetAsyncKeyState(vk) & 0x8000) != 0;
        }
    }
}

BindManager *bindManagerCreate(Controller *controller) {
    if (!controller) return NULL;
    BindManager *manager = (BindManager *)calloc(1, sizeof(BindManager));
    if (!manager) {
        LOG_ERROR("Couldn't allocate BindManager");
        return NULL;
    }
    manager->controller = controller;
    manager->commandManager = controllerGetCommandManager(controller);
    loadBinds(manager);
    return manager;
}

void bindManagerDestroy(BindManager *manager) {
    if (!manager) return;
    free(manager);
}

void bindManagerReload(BindManager *manager) {
    if (!manager) return;
    loadBinds(manager);
}

void bindManagerUpdate(BindManager *manager) {
    if (!manager) return;

    // Command-history poll for the in-game chat arrow keys (always runs).
    manager->commandManager = controllerGetCommandManager(manager->controller);
    commandManagerUpdate(manager->commandManager);

    // Gate: game attached, window focused, chat/pause closed.
    if (!controllerIsGameAttached(manager->controller)) return;
    if (!controllerIsGameWindowFocused(manager->controller)) return;
    if (controllerIsChatOpen(manager->controller) || controllerIsZombiesGamePaused(manager->controller)) return;

    for (int i = 0; i < manager->bindCount; i++) {
        int vk = manager->binds[i].vkCode;
        if (!isValidVKCode(vk)) continue;
        bool pressed = (GetAsyncKeyState(vk) & 0x8000) != 0;
        if (pressed && !manager->prevKeyStates[vk]) {
            executeCommands(manager, manager->binds[i].command);
            LOG_DEBUG("Keybind executed: %s -> %s", manager->binds[i].keyName, manager->binds[i].command);
        }
        manager->prevKeyStates[vk] = pressed;
    }
}
