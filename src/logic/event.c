#include "logic/event.h"
#include "logic/gsc.h"
#include "controller.h"
#include "controller/controller_internal.h"
#include "logic/game.h"
#include "logic/cheat/manager.h"
#include "logic/cheat/manager/handlers.h"
#include "logger.h"
#include "win/process.h"
#include "logic/command.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static Controller *controller;
static GSC *gsc;

static bool eventHandleChatMessage(Event event);
static bool eventHandleMapChange(Event event);
static bool eventHandleMapRestart(Event event);
static bool eventHandleVMNotify(Event event);
static bool eventHandleIDUpdate(Event event);

static bool _eventValidIDUpdate(int eventId, int *pEventValue);

void eventInit(Controller *controllerInstance) {
    controller = controllerInstance;
    gsc = _controllerGetGsc(controller);
    commandInit(controller);
}

Event eventPoll() {
    Process *process = controllerGetProcess(controller);
    Event event = processPollFromPipe(process); // Blocking call
    return event;
}

bool eventHandle(Event event) {
    switch (event.type) {
        case EVENT_CHAT_MESSAGE: return eventHandleChatMessage(event);
        case EVENT_MAP_CHANGE: return eventHandleMapChange(event);
        case EVENT_MAP_RESTART: return eventHandleMapRestart(event);
        case EVENT_VM_NOTIFY: return eventHandleVMNotify(event);
        case EVENT_ID_UPDATE: return eventHandleIDUpdate(event);
        case EVENT_INVALID:
        default:
            LOG_ERROR("Unknown event %d\n", event.type);
            return false;
    }
}

static bool eventHandleChatMessage(Event event) {
    Command command = commandBuild(event.data.chat.message);
    return commandHandle(command);
}

static bool eventHandleMapChange(Event event) {
    State *state = controllerGetState(controller);
    Game *game = &state->activeGame;
    gameEnd(game, event.timestamp);
    gameClear(game);
    return true;
}

static bool eventHandleMapRestart(Event event) {
    State *state = controllerGetState(controller);
    Game *game = &state->activeGame;
    gameEnd(game, event.timestamp);
    gameClear(game);
    return true;
}

static bool eventHandleVMNotify(Event event) {
    State *state = controllerGetState(controller);
    Game *game = &state->activeGame;
    if (strcmp(event.data.vmNotify.eventName, "fade_introblack") == 0) {
        cheatManagerHandleGameStart(_controllerGetCheatManager(controller));
        return true;
    }
    if (strcmp(event.data.vmNotify.eventName, "fade_in_complete") == 0) {
        return gameStart(game, controllerGetLevelName(controller), event.timestamp);
    }
    if (strcmp(event.data.vmNotify.eventName, "start_of_round") == 0) {
        return gameRoundStarted(game, event.timestamp);
    }
    if (strcmp(event.data.vmNotify.eventName, "end_of_round") == 0) {
        return gameRoundEnded(game, event.timestamp);
    }
    if (strncmp(event.data.vmNotify.eventName, "bo1zt::Level::TotalZombiesKilled", 32) == 0) {
        int currentZombies = game->totalZombies;
        int totalZombiesKilled = event.data.vmNotify.eventValue;
        int zombiesKilledThisFrame = totalZombiesKilled - currentZombies;
        for (int i = 0; i < zombiesKilledThisFrame; i++) {
            gameZombieKilled(game);
        }
        return true;
    }
    if (strncmp(event.data.vmNotify.eventName, "bo1zt::Level::Powerup::Dropped", 30) == 0) {
        Powerup powerup = (Powerup)event.data.vmNotify.eventValue;
        return gamePowerupDropped(game, powerup);
    }
    if (strncmp(event.data.vmNotify.eventName, "bo1zt::Level::Powerup::NewCycle", 31) == 0) {
        gamePowerupNewCycle(game);
        return true;
    }
    if (strncmp(event.data.vmNotify.eventName, "bo1zt::Worker", 13) == 0) {
        int index;
        char response[256];
        if (sscanf(event.data.vmNotify.eventName, "bo1zt::Worker%d::%255s", &index, response) == 2) {
            gscWriteResponse(gsc, index, response);
        }
        return true;
    }
    return true;
}

// Not in use since GSC was introduced.
static bool eventHandleIDUpdate(Event event) {
    Process *process = controllerGetProcess(controller);
    State *state = controllerGetState(controller);
    Game *game = &state->activeGame;
    int eventId = event.data.idUpdate.eventId;
    int *pEventValue =  event.data.idUpdate.pEventValue;
    if (!_eventValidIDUpdate(eventId, pEventValue)) return true;
    switch (eventId) {
        // Round
        case 4748:
        default:
            return true;
    }
}

static bool _eventValidIDUpdate(int eventId, int *pEventValue) {
    if (!pEventValue) return NULL;
    Process *process = controllerGetProcess(controller);
    int value;
    processRead(process, (uint32_t)pEventValue + 0x8, &value, sizeof(int));
    // Some ID Updates are sent multiple times with invalid values. The event contains its ID on the value so we must check it's the same.
    return value >> 0x8 == eventId;
}
