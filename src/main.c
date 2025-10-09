#include <stdio.h>
#include "gui/gui.h"
#include "memory/memory.h"
#include "controller/controller.h"
#include "logger/logger.h"

int main(void) {
    Controller *controller = controllerCreate();
    loggerInit(controller);
    if (!controller) {
        LOG_ERROR("Could not create controller. Make sure the game is running.\n");
        return 1;
    }
    guiInit(controller);
    guiRun();
    guiCleanup();
    return 0;
}