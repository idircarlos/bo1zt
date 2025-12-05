#include "event.h"
#include "../server/server.h"
#include "../logger/logger.h"
#include "../process/process.h"
#include "../controller/controller_internal.h"

static Controller *controller;
static Server *server;

void eventInit(Controller *controllerInstance) {
    controller = controllerInstance;
    server = _controllerGetServer(controller);
}

Event eventPoll() {
    Process *process = controllerGetProcess(controller);
    Event event = processPollFromPipe(process); // Blocking call
    return event;
}

bool eventHandle(Event event) {
    switch (event.type) {
        case EVENT_CHAT_MESSAGE:
            LOG_INFO("Chat message received: %s\n", event.data);
            break;
        case EVENT_MAP_CHANGE:
            LOG_INFO("Map changed to: %s\n", event.data);
            break;
        case EVENT_MAP_RESTART:
            LOG_INFO("Map restarted %s\n", event.data);
            break;
        case EVENT_VM_NOTIFY:
            LOG_INFO("VM notify: %s\n", event.data);
            break;
        case EVENT_ID_UPDATE:
            LOG_INFO("ID updated: %s\n", event.data);
            break;
        case EVENT_INVALID:
        default:
            return false;
    }
    return true;
}
