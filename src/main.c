#include <stdio.h>
#include "gui/gui.h"
#include "memory/memory.h"
#include "controller/controller.h"
#include "logger/logger.h"
#include "thread/thread.h"

static Controller *controller = NULL;

int threadEntryPointTest(void *data) {
    (void)data;
    while(true) {
        while(!controllerIsGameRunning(controller)) {
            LOG_INFO("Waiting for game running...\n");
            threadSleep(3000);
        }
        LOG_INFO("Game attached! Waiting until game is closed...\n");
        controllerWaitUntilGameCloses(controller);
        LOG_INFO("Game has been closed\n");
    }
    
    return 0;
}

int main(void) {
    controller = controllerCreate();
    loggerInit(controller);
    threadCreate(threadEntryPointTest, NULL);
    guiInit(controller);
    guiRun();
    guiCleanup();
    
    return 0;
}