#include "logic/event.h"
#include "logic/gsc.h"
#include "controller/controller.h"
#include "controller/controller_internal.h"
#include "logic/game.h"
#include "logger/logger.h"
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
    Game *game = state->activeGame;
    gameEnd(game, event.timestamp);
    gameClear(game);
    return true;
}

static bool eventHandleMapRestart(Event event) {
    State *state = controllerGetState(controller);
    Game *game = state->activeGame;
    gameEnd(game, event.timestamp);
    gameClear(game);
    return true;
}

static bool eventHandleVMNotify(Event event) {
    State *state = controllerGetState(controller);
    Game *game = state->activeGame;
    if (strcmp(event.data.vmNotify.eventName, "fade_in_complete") == 0) {
        return gameStart(game, controllerGetLevelName(controller), event.timestamp);
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

static bool eventHandleIDUpdate(Event event) {
    Process *process = controllerGetProcess(controller);
    State *state = controllerGetState(controller);
    Game *game = state->activeGame;
    int eventId = event.data.idUpdate.eventId;
    int *pEventValue =  event.data.idUpdate.pEventValue;
    if (!_eventValidIDUpdate(eventId, pEventValue)) return true;
    switch (eventId) {
        // Round
        case 4748:
            return processRead(process, (uint32_t)pEventValue + 0x4, &(game->currentRound->number), sizeof(int));
        default:
            return true;
    }
}

static bool _eventValidIDUpdate(int eventId, int *pEventValue) {
    Process *process = controllerGetProcess(controller);
    int value;
    processRead(process, (uint32_t)pEventValue + 0x8, &value, sizeof(int));
    // Some ID Updates are sent multiple times with invalid values. The event contains its ID on the value so we must check it's the same.
    return value >> 0x8 == eventId;
}
