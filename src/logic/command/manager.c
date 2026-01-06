#include "logic/command.h"
#include "logic/command/manager.h"
#include "logic/command/manager/manager_internal.h"
#include "logic/command/graphics.h"
#include "logic/command/hacks.h"
#include "logic/command/misc.h"
#include "controller.h"
#include "utils/map.h"
#include <stdlib.h>
#include <windows.h>

typedef struct {
    CommandIteratorFn userFn;
    void *userData;
} CommandForEachContext;

static CommandEntry *createEntry(CommandName name, const char *usage, const char *description) {
    CommandEntry *entry = (CommandEntry *)malloc(sizeof(CommandEntry));
    if (!entry) return NULL;
    entry->name = name;
    entry->usage = usage;
    entry->description = description;
    return entry;
}

static void registerCommand(Map *map, const char *cmd, CommandName name, 
                           const char *usage, const char *description) {
    CommandEntry *entry = createEntry(name, usage, description);
    if (entry) mapPut(map, cmd, entry);
}

static Map *createCommandsMap(void) {
    Map *map = mapCreate();

    // Hacks
    registerCommand(map, "god", COMMAND_GOD, "/god", "Toggle god mode");
    registerCommand(map, "noclip", COMMAND_NOCLIP, "/noclip", "Toggle noclip");
    registerCommand(map, "invis", COMMAND_INVISIBLE, "/invis", "Toggle invisibility");
    registerCommand(map, "infammo", COMMAND_INFAMMO, "/infammo", "Toggle infinite ammo");
    registerCommand(map, "insta", COMMAND_INSTA, "/insta", "Toggle instant kill");
    registerCommand(map, "norecoil", COMMAND_NORECOIL, "/norecoil", "Toggle no recoil");
    registerCommand(map, "noshellshock", COMMAND_NOSHELLSHOCK, "/noshellshock", "Toggle no shellshock");
    registerCommand(map, "speed", COMMAND_SPEED, "/speed", "Toggle fast gameplay");
    registerCommand(map, "knife", COMMAND_KNIFE, "/knife", "Toggle increased knife range");
    registerCommand(map, "crosshair", COMMAND_CROSSHAIR, "/crosshair", "Toggle small crosshair");
    registerCommand(map, "staticbox", COMMAND_STATICBOX, "/staticbox", "Toggle box never moves");
    registerCommand(map, "thirdperson", COMMAND_THIRDPERSON, "/thirdperson", "Toggle third person view");

    // Graphics
    registerCommand(map, "fov", COMMAND_FOV, "/fov <value>", "Set field of view");
    registerCommand(map, "fovscale", COMMAND_FOVSCALE, "/fovscale <value>", "Set FOV scale");
    registerCommand(map, "fps", COMMAND_FPS, "/fps <value>", "Set FPS cap");
    registerCommand(map, "unlimitfps", COMMAND_UNLIMITFPS, "/unlimitfps", "Toggle unlimited FPS");
    registerCommand(map, "borderless", COMMAND_BORDERLESS, "/borderless", "Toggle borderless window");
    registerCommand(map, "disablehud", COMMAND_DISABLEHUD, "/disablehud", "Toggle HUD visibility");
    registerCommand(map, "disablefog", COMMAND_DISABLEFOG, "/disablefog", "Toggle fog visibility");
    registerCommand(map, "fullbright", COMMAND_FULLBRIGHT, "/fullbright", "Toggle fullbright");
    registerCommand(map, "colorized", COMMAND_COLORIZED, "/colorized", "Toggle colorized mode");

    // GSC
    registerCommand(map, "perk", COMMAND_PERK, "/perk <add | rm> <perks>", "Add/remove perks (jg qr sc dt su mk)");

    // Misc
    registerCommand(map, "give", COMMAND_GIVE, "/give <ammo | weapon>", "Give ammo or weapon (ray tg bow mk bh)");
    registerCommand(map, "tp", COMMAND_TP, "/tp [x y z]", "Teleport to coordinates");
    registerCommand(map, "restart", COMMAND_RESTART, "/restart", "Restart the map");
    registerCommand(map, "music", COMMAND_MUSIC, "/music", "Play easter egg song");
    registerCommand(map, "uwu", COMMAND_UWU, "/uwu", "UwU");

    // Special rounds
    registerCommand(map, "dogs", COMMAND_DOGS, "/dogs", "Show next dog round");
    registerCommand(map, "monkeys", COMMAND_MONKEYS, "/monkeys", "Show next monkey round");
    registerCommand(map, "thief", COMMAND_THIEF, "/thief", "Show next thief round");

    // Info
    registerCommand(map, "claymores", COMMAND_CLAYMORES, "/claymores", "Show claymore count");
    registerCommand(map, "entities", COMMAND_ENTITIES, "/entities", "Show entity count");
    registerCommand(map, "sph", COMMAND_SPH, "/sph [round]", "Show seconds per horde");
    registerCommand(map, "trade", COMMAND_TRADE, "/trade [start | end | cancel | total]", "Manage trade timer");
    registerCommand(map, "revives", COMMAND_REVIVES, "/revives", "Show quick revives drunk");

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
    manager->commandsMap = createCommandsMap();
    manager->history = historyCreate();
    manager->prevState.chatOpen = false;
    manager->prevState.upKey = false;
    manager->prevState.downKey = false;
    manager->submodulesInitialized = false;

    return manager;
}

void commandManagerInitSubmodules(CommandManager *manager) {
    if (!manager || manager->submodulesInitialized) return;
    initSubmodules(manager);
    manager->submodulesInitialized = true;
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
    CommandEntry *entry = (CommandEntry *)mapGet(manager->commandsMap, cmd);
    return entry ? entry->name : COMMAND_UNKNOWN;
}

static void forEachAdapter(const char *key, void *value, void *userData) {
    (void)key;
    CommandForEachContext *ctx = (CommandForEachContext *)userData;
    CommandEntry *entry = (CommandEntry *)value;
    ctx->userFn(entry, ctx->userData);
}

void commandManagerForEach(CommandManager *manager, CommandIteratorFn fn, void *userData) {
    if (!manager || !fn) return;
    CommandForEachContext ctx = { fn, userData };
    mapForEach(manager->commandsMap, forEachAdapter, &ctx);
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
