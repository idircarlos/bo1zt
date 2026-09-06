#include <stdio.h>
#include <sys/stat.h>
#include "gui.h"
#include "gui/game.h"
#include "gui/assets.h"
#include "ipc/event.h"
#include "logger.h"
#include "win/process.h"
#include "controller.h"
#include "logic/config.h"
#include "win/file.h"
#include "win/thread.h"
#include "logic/event.h"
#include "win/resources.h"
#include "resource_ids.h"
#include "service.h"
#include "client.h"
#include "cli.h"

static Controller *controller = NULL;
static Service *service = NULL;

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
        LOG_INFO("Game attached! Waiting for its window");
        while (!controllerIsGameWindowAttached(controller)) {
            // This can happen if the game exits before attaching the window
            processExited = !controllerIsGameRunning(controller);
            if (processExited) {
                LOG_INFO("Game exited before its window was attached");
                controllerDetachGame(controller);
                break;
            }
            controllerTryAttachGameWindow(controller);
            threadSleep(200);
        }
        if (processExited) continue;
        LOG_INFO("Game window attached!");
        while (!controllerIsGameReady(controller)) {
            // This can happen if the game exits before being ready
            processExited = !controllerIsGameRunning(controller);
            if (processExited) {
                LOG_INFO("Game exited before becoming ready");
                controllerDetachGame(controller);
                break;
            }
            
            threadSleep(200);
        }
        if (processExited) continue;
        LOG_INFO("Game ready!");
        Process *process = controllerGetProcess(controller);

        char gscPath[MAX_PATH];
        if (fileAppFolderPath(gscPath, sizeof(gscPath), "gsc")) {
            resourcesExtractZip(IDR_GSC_ZIP, gscPath);
        }

        // Inject DLL. Add some delay to let the game completly load and avoid random crashes
        threadSleep(1000);
        if (!processInjectDll(process, DLL_NAME)) {
            LOG_ERROR("Failed to inject the DLL. Game events will be unavailable");
        } else {
            threadSleep(500); // Wait a bit to let the DLL initialize the pipe
            processConnectPipe(process);
        }
        controllerInitTrainerConfig(controller);
        controllerWaitUntilGameCloses(controller);
        LOG_INFO("Game closed");
        controllerDetachGame(controller);
    }
    return 0;
}

int updateGameThread(void *data) {
    (void)data;
    // For some reason, there is a bug that UI components are return random values from others threads immediately after building the UI.
    // Waiting a bit as a workaround.
    threadSleep(1000);
    while (true) {
        controllerUpdateState(controller);
        controllerUpdateTrainerConfig(controller);
        controllerUpdateManagers(controller);
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
        char dir[MAX_PATH];
        if (!uiGamePromptLocation(dir, sizeof(dir))) {
            uiQuit();
            exit(0);
        }
        Config *config = controllerGetConfig(controller);
        strncpy(config->game.location, dir, sizeof(config->game.location) - 1);
        config->game.location[sizeof(config->game.location) - 1] = '\0';
        configSave(config);
    }
}

static void extractGameAssetsAware() {
    GameConfig gameConfig = controllerGetGameConfig(controller);
    uiAssetsInstall(gameConfig.location);
}


int main(int argc, char **argv) {
    if (argc > 1) {
        return cliMain(argc, argv);
    }

    loggerInit(NULL);
    controller = controllerCreate();
    setupResources();

    controllerInitManagers(controller);

    service = serviceCreate(controller);
    int port = serviceResolvePort();
    serviceServe(service, port);

    Client *client = clientCreate(port);
    for (int i = 0; i < 100; i++) {
        char version[64];
        if (clientGetVersion(client, version, sizeof(version)) == CLIENT_OK) break;
        threadSleep(20);
    }

    guiInit(client, port);

    promptGameLocationAware();
    extractGameAssetsAware();

    threadCreate(processRunningThread, NULL);
    threadCreate(updateGameThread, NULL);
    threadCreate(eventHandlerThread, NULL);
    guiRun();
    guiCleanup();
    clientDestroy(client);
    return 0;
}