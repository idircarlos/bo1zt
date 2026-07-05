#include "logic/command/hacks.h"
#include "client.h"
#include "client/cheats.h"
#include "service.h"

static int clientPort;

void commandHacksInit(Controller *controllerInstance) {
    (void)controllerInstance;
    clientPort = serviceResolvePort();
}

static bool toggle(CheatName cheat) {
    Client *client = clientCreate(clientPort);
    if (!client) return false;

    bool enabled = false;
    bool ok = false;
    if (clientGetCheat(client, cheat, &enabled) == CLIENT_OK) {
        ok = clientSetCheat(client, cheat, !enabled) == CLIENT_OK;
    }

    clientDestroy(client);
    return ok;
}

bool commandNoclipHandle(Command command) { (void)command; return toggle(CHEAT_NAME_NO_CLIP); }
bool commandGodHandle(Command command) { (void)command; return toggle(CHEAT_NAME_GOD_MODE); }
bool commandInvisibleHandle(Command command) { (void)command; return toggle(CHEAT_NAME_INVISIBLE); }
bool commandInfammoHandle(Command command) { (void)command; return toggle(CHEAT_NAME_INFINITE_AMMO); }
bool commandInstaHandle(Command command) { (void)command; return toggle(CHEAT_NAME_INSTANT_KILL); }
bool commandNorecoilHandle(Command command) { (void)command; return toggle(CHEAT_NAME_NO_RECOIL); }
bool commandCrosshairHandle(Command command) { (void)command; return toggle(CHEAT_NAME_SMALL_CROSSHAIR); }
bool commandSpeedHandle(Command command) { (void)command; return toggle(CHEAT_NAME_FAST_GAMEPLAY); }
bool commandNoshellshockHandle(Command command) { (void)command; return toggle(CHEAT_NAME_NO_SHELLSHOCK); }
bool commandKnifeHandle(Command command) { (void)command; return toggle(CHEAT_NAME_INCREASE_KNIFE_RANGE); }
bool commandStaticboxHandle(Command command) { (void)command; return toggle(CHEAT_NAME_BOX_NEVER_MOVES); }
bool commandThirdpersonHandle(Command command) { (void)command; return toggle(CHEAT_NAME_THIRD_PERSON); }
