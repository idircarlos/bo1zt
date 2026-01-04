#include <stdio.h>
#include <sys/stat.h>
#include "gui.h"
#include "gui/game.h"
#include "ipc/event.h"
#include "logger.h"
#include "win/process.h"
#include "controller.h"
#include "logic/config.h"
#include "win/thread.h"
#include "logic/event.h"
#include "win/resources.h"
#include "resource_ids.h"

static Controller *controller = NULL;

int processRunningThread(void *data) {
    (void)data;
    bool processExited = false;
    while (true) {
        LOG_INFO("Waiting for game to start...");
        while(!controllerIsGameRunning(controller)) {
            threadSleep(500);
        }
        if (!controllerIsGameAttached(controller)) {
            controllerAttachGame(controller);
        }
        LOG_INFO("Game attached! Looking for game window");
        while (!controllerIsGameWindowAttached(controller)) {
            // This can happen if the game exits before attaching the window
            processExited = !controllerIsGameRunning(controller);
            if (processExited) {
                LOG_INFO("Game exited before attaching the window");
                controllerDetachGame(controller);
                break;
            }
            controllerTryAttachGameWindow(controller);
            threadSleep(200);
        }
        if (processExited) continue;
        LOG_INFO("Window attached!");
        while (!controllerIsGameReady(controller)) {
            // This can happen if the game exits before being ready
            processExited = !controllerIsGameRunning(controller);
            if (processExited) {
                LOG_INFO("Game exited before being ready");
                controllerDetachGame(controller);
                break;
            }
            
            threadSleep(200);
        }
        if (processExited) continue;
        LOG_INFO("Game ready!");
        Process *process = controllerGetProcess(controller);
        GameConfig gameConfig = controllerGetGameConfig(controller);
        if (strlen(gameConfig.location) == 0) {
            LOG_WARN("Game location not configured. DLL injection skipped. Use 'Launch Game' button to configure.");
        } else {
            // Extract GSC scripts to game directory
            char gscPath[MAX_PATH + 16];
            snprintf(gscPath, sizeof(gscPath), "%s\\bo1zt\\gsc", gameConfig.location);
            resourcesExtractZip(IDR_GSC_ZIP, gscPath);
            
            // Inject DLL
            if (!processInjectDll(process, DLL_NAME, gameConfig.location)) {
                LOG_ERROR("Failed to inject DLL into game process. Events won't be received.");
            } else {
                Sleep(500); // Wait a bit to let the DLL initialize the pipe
                processConnectPipe(process);
            }
        }
        controllerInitTrainerConfig(controller);
        controllerWaitUntilGameCloses(controller);
        LOG_INFO("Game has been closed");
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
        if (event.type != EVENT_INVALID) {
            eventHandle(event);
        }
    }
    return 0;
}

static void setupResources() {
    resourcesInit();
    resourcesLoadFont(IDR_FONT_DIGITAL_7_MONO);
}

static void promptGameLocationAware() {
    GameConfig gameConfig = controllerGetGameConfig(controller);
    if (strlen(gameConfig.location) == 0) {
        if (!uiGamePromptLocation()) {
            uiQuit();
            exit(0);
        }
    }
}


int main(void) {
    loggerInit(NULL);
    controller = controllerCreate();
    setupResources();
    guiInit(controller);
    
    // Prompt game location MsgBox if first time using the app
    promptGameLocationAware();
    
    threadCreate(processRunningThread, NULL);
    threadCreate(updateGameThread, NULL);
    threadCreate(eventHandlerThread, NULL);
    guiRun();
    guiCleanup();
    return 0;
}