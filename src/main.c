#include <stdio.h>
#include "gui/gui.h"
#include "process/process.h"
#include "controller/controller.h"
#include "logger/logger.h"
#include "thread/thread.h"
#include "command/command.h"

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
        while (!controllerIsGameReady(controller)) {
            // This can happen if the game exits before being ready
            processExited = !controllerIsGameRunning(controller);
            if (processExited) {
                LOG_INFO("Game exited before being ready\n");
                controllerDetachGame(controller);
                break;
            }
            
            threadSleep(200);
        }
        if (processExited) continue;
        LOG_INFO("Game ready!\n");
        controllerInitTrainerConfig(controller);
        controllerWaitUntilGameCloses(controller);
        LOG_INFO("Game has been closed\n");
        controllerDetachGame(controller);
    }
    return 0;
}

int updateGameThread(void *data) {
    (void)data;
    // For some reason, there is a bug that UI components are return random values from others threads immediately after the building the UI.
    // Waiting a bit as a workaround.
    threadSleep(1000);
    while (true) {
        controllerUpdateState(controller);
        controllerUpdateTrainerConfig(controller);
        threadSleep(1000/60); // 60 FPS
    }
}

int commandHandlerThread(void *data) {
    (void)data;
    while (!controllerIsGameReady(controller)) {
        threadSleep(200);
    }
    Server *server = serverCreate(controller);
    commandInit(controller, server);
    while (true) {
        if (!controllerIsGameReady(controller)) continue;
        Command *command = commandPoll();   // Blocking call. Waits until a command is available.
        commandHandle(command);
        commandFree(command);
    }
}


int main(void) {
    loggerInit(NULL);
    controller = controllerCreate();
    guiInit(controller);
    threadCreate(processRunningThread, NULL);
    threadCreate(updateGameThread, NULL);
    threadCreate(commandHandlerThread, NULL);
    guiRun();
    guiCleanup();
    return 0;
}