#include <sys/stat.h>
#include "gui/gui.h"
#include "process/process.h"
#include "controller/controller.h"
#include "logger/logger.h"
#include "thread/thread.h"
#include "command/command.h"
#include "event/event.h"
#include "resources/resources.h"
#include "../res/resource_ids.h"

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
        Process *process = controllerGetProcess(controller);
        if (!processInjectDll(process, DLL_NAME)) {
            LOG_ERROR("Failed to inject DLL into game process. Events won't be received.\n");
        } else {
            Sleep(500); // Wait a bit to let the DLL initialize the pipe
            processConnectPipe(process);
        }
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
    commandInit(controller);
    while (true) {
        if (!controllerIsGameReady(controller)) continue;
        Command *command = commandPoll();   // Blocking call. Waits until a command is available.
        commandHandle(command);
        commandFree(command);
    }
}

int eventHandlerThread(void *data) {
    (void)data;
    Process *process = NULL;
    while (!controllerIsGameReady(controller)) {
        threadSleep(200);
    }
    
    eventInit(controller);
    while (true) {
        process = controllerGetProcess(controller);
        if (!controllerIsGameReady(controller) || !processIsPipeConnected(process)) {
            threadSleep(200);
            continue;
        }
        Event event = eventPoll();  // Blocking call. Waits until an event is available.
        eventHandle(event);
    }
    return 0;
}

static void setupResources() {
    resourcesInit();
    resourcesLoadFont(IDR_FONT_DIGITAL_7_MONO);
}


int main(void) {
    loggerInit(NULL);
    controller = controllerCreate();
    setupResources();
    guiInit(controller);
    threadCreate(processRunningThread, NULL);
    threadCreate(updateGameThread, NULL);
    threadCreate(commandHandlerThread, NULL);
    threadCreate(eventHandlerThread, NULL);
    guiRun();
    guiCleanup();
    return 0;
}