#include "logic/event.h"
#include "controller/controller.h"
#include "logic/game.h"
#include "logic/server.h"
#include "logger/logger.h"
#include "win/process.h"
#include "logic/command.h"
#include "controller/controller_internal.h"
#include <string.h>

static Controller *controller;
static Server *server;

static bool eventHandleChatMessage(Event event);
static bool eventHandleMapRestart(Event event);
static bool eventHandleVMNotify(Event event);

void eventInit(Controller *controllerInstance) {
    controller = controllerInstance;
    server = _controllerGetServer(controller);
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
        case EVENT_MAP_CHANGE:
            LOG_INFO("Map changed to: %s\n", event.data.mapChange.mapName);
            break;
        case EVENT_MAP_RESTART: return eventHandleMapRestart(event);
            break;
        case EVENT_VM_NOTIFY: return eventHandleVMNotify(event);
        case EVENT_ID_UPDATE:
            LOG_INFO("ID updated: %u = %d\n", event.data.idUpdate.eventId, event.data.idUpdate.pEventValue);
            break;
        case EVENT_INVALID:
        default:
            return false;
    }
    return true;
}

static bool eventHandleChatMessage(Event event) {
    Command command = commandBuild(event.data.chat.message);
    return commandHandle(command);
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
    if (strcmp(event.data.vmNotify.eventName, "start_of_round") == 0) {
        return roundStart(game->currentRound, event.timestamp);
    }
    return true;
}
