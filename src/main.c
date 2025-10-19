#include <stdio.h>
#include "gui/gui.h"
#include "memory/memory.h"
#include "controller/controller.h"
#include "logger/logger.h"
#include "thread/thread.h"

static Controller *controller = NULL;

int processRunningThread(void *data) {
    (void)data;
    while (true) {
        while(!controllerIsGameRunning(controller)) {
            LOG_INFO("Waiting for game running...\n");
            threadSleep(3000);
        }
        controllerAttachGame(controller);
        LOG_INFO("Game attached! Waiting until game is closed...\n");
        LOG_INFO("active = %d\t resets = %d\n", controllerIsZombiesGameActive(controller), controllerGetGameResets(controller));
        controllerWaitUntilGameCloses(controller);
        LOG_INFO("Game has been closed\n");
    }
    return 0;
}

int refreshWindowThread(void *data) {
    (void)data;
    while (true) {
        while(!controllerIsGameRunning(controller)) {
            threadSleep(3000);
        }
        guiUpdate(controller);
        threadSleep(1000);
    }
}

int main(void) {
    controller = controllerCreate();
    loggerInit(controller);
    threadCreate(processRunningThread, NULL);
    guiInit(controller);
    threadCreate(refreshWindowThread, NULL);
    guiRun();
    guiCleanup();
    
    return 0;
}