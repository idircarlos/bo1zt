#include <stdio.h>
#include "gui/gui.h"
#include "process/process.h"
#include "controller/controller.h"
#include "logger/logger.h"
#include "thread/thread.h"
#include <windows.h>

static Controller *controller = NULL;

int processRunningThread(void *data) {
    (void)data;
    while (true) {
        LOG_INFO("Waiting for game starts...\n");
        while(!controllerIsGameRunning(controller)) {
            threadSleep(500);
        }
        if (!controllerIsGameAttached(controller)) {
            controllerAttachGame(controller);
        }
        LOG_INFO("Game attached! Looking for Game Window\n");
        while (!controllerIsGameWindowAttached(controller)) {
            controllerTryAttachGameWindow(controller);
            threadSleep(200);
        }
        LOG_INFO("Window attached!\n");
        controllerWaitUntilGameCloses(controller);
        LOG_INFO("Game has been closed\n");
        controllerDetachGame(controller);
    }
    return 0;
}

int refreshWindowThread(void *data) {
    (void)data;
    while (true) {
        while(!controllerIsGameRunning(controller)) {
            threadSleep(500);
        }
        guiUpdate();
        threadSleep(500);
    }
    return 0;
}


int main(void) {
    loggerInit(NULL);
    controller = controllerCreate();
    threadCreate(processRunningThread, NULL);
    guiInit(controller);
    threadCreate(refreshWindowThread, NULL);
    guiRun();
    guiCleanup();
    
    return 0;
}