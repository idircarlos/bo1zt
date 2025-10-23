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
        while(!controllerIsGameRunning(controller)) {
            LOG_INFO("Waiting for game running...\n");
            threadSleep(3000);
        }
        if (!controllerIsGameAttached(controller)) {
            controllerAttachGame(controller);
        }
        LOG_INFO("Game attached! Waiting until game is closed...\n");
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
            threadSleep(3000);
        }
        guiUpdate();
        threadSleep(1000);
    }
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