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
    bool processExited = false;
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
            // This can happen if the game exits before attaching the window
            processExited = !controllerIsGameRunning(controller);
            if (processExited) {
                LOG_INFO("Game exited before attaching the window\n");
                controllerDetachGame(controller);
                break;
            }
            controllerTryAttachGameWindow(controller);
            threadSleep(200);
        }
        if (processExited) continue;
        LOG_INFO("Window attached!\n");
        controllerInitTrainerConfig(controller);
        controllerWaitUntilGameCloses(controller);
        LOG_INFO("Game has been closed\n");
        controllerDetachGame(controller);
    }
    return 0;
}

int updateGameThread(void *data) {
    (void)data;
    while (true) {
        controllerUpdateState(controller);
        guiUpdate();
        controllerUpdateTrainerConfig(controller);
        threadSleep(1000);
    }
}


int main(void) {
    loggerInit(NULL);
    controller = controllerCreate();
    guiInit(controller);
    threadCreate(processRunningThread, NULL);
    threadCreate(updateGameThread, NULL);
    guiRun();
    guiCleanup();
    return 0;
}