#include "event.h"
#include "../server/server.h"
#include "../logger/logger.h"
#include "../process/process.h"
#include "../command/command.h"
#include "../controller/controller_internal.h"

static Controller *controller;
static Server *server;

static void eventHandleChatMessage(Event event);

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
        case EVENT_CHAT_MESSAGE:
            eventHandleChatMessage(event);
            break;
        case EVENT_MAP_CHANGE:
            LOG_INFO("Map changed to: %s\n", event.data.mapChange.mapName);
            break;
        case EVENT_MAP_RESTART:
            LOG_INFO("Map restarted at timestamp: %u\n", event.timestamp);
            break;
        case EVENT_VM_NOTIFY:
            LOG_INFO("VM notify: %s = %d\n", event.data.vmNotify.eventName, event.data.vmNotify.eventValue);
            break;
        case EVENT_ID_UPDATE:
            LOG_INFO("ID updated: %u = %d\n", event.data.idUpdate.eventId, event.data.idUpdate.pEventValue);
            break;
        case EVENT_INVALID:
        default:
            return false;
    }
    return true;
}

static void eventHandleChatMessage(Event event) {
    Command command = commandBuild(event.data.chat.message);
    commandHandle(command);
}
