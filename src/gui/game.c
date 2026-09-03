#include "gui/game.h"
#include "gui.h"
#include "gui/widgets.h"
#include "gui/binds.h"
#include "gui/camo.h"
#include "gui/gsc.h"
#include "gui/logs.h"
#include "gui/twitch.h"
#include "client/cheats.h"
#include "client/game.h"
#include "client/round.h"
#include "logger.h"
#include "logic/cheat.h"
#include "win/thread.h"
#include "resource_ids.h"
#include <windows.h>
#include <stdbool.h>
#include <string.h>
#include <ui.h>
#include <stdio.h>
#include <stdlib.h>

#define GAME_EXECUTABLE_NAME "BlackOps.exe"

static bool gameAttached = false;

// Shared HTTP client
static Client *client;
static uiWindow *parent;

// UI Elements
static uiCheckbox *patchMovementCheckbox = NULL;
static uiCheckbox *showFpsCheckbox = NULL;
static uiLabel *hostnameLabel = NULL;
static uiEntry *hostnameEntry = NULL;
static uiButton *changeRoundButton = NULL;
static uiSpinbox *changeRoundSpin = NULL;
static uiButton *bindsButton = NULL;
static uiButton *widgetsButton = NULL;
static uiButton *camoButton = NULL;
static uiButton *twitchButton = NULL;
static uiButton *gscButton = NULL;
static uiButton *logsButton = NULL;
static uiButton *launchButton = NULL;
static uiButton *closeButton = NULL;

static uiEntry *locationEntry = NULL;

static uiWindow *widgetsWindow = NULL;

static uiWindow *bindsWindow = NULL;

static int threadLaunchGame(void *data) {
    (void)data;
    LOG_INFO("Launching game from UI");
    uiControlDisable(uiControl(launchButton));
    Client *localClient = clientCreate(guiClientPort());
    bool success = localClient && clientLaunchGame(localClient) == CLIENT_OK;
    clientDestroy(localClient);
    if (!success) {
        uiMsgBoxError(parent, "Launch game", "Couldn't launch Call of Duty Black Ops 1. Location may have changed or the game is already running.");
        return 1;
    }
    return 0;
}

static int threadCloseGame(void *data) {
    (void)data;
    LOG_INFO("Closing game from UI");
    uiControlDisable(uiControl(closeButton));
    Client *localClient = clientCreate(guiClientPort());
    bool success = localClient && clientCloseGame(localClient) == CLIENT_OK;
    clientDestroy(localClient);
    if (!success) {
        uiMsgBoxError(parent, "Close game", "Couldn't close Call of Duty Black Ops 1. Game is probably already closed.");
        return 1;
    }
    return 0; 
}

static int onLaunchGameError(void *data) {
    (void)data;
    uiMsgBoxError(parent, "Launch game", "Couldn't launch Call of Duty Black Ops 1. Probably is either running or stuck at launch in Steam. Try killing the game from Steam or Task Manager.");
    uiControlEnable(uiControl(launchButton));
    return 0;
}

static int onCloseGameError(void *data) {
    (void)data;
    uiMsgBoxError(parent, "Close game", "Couldn't close Call of Duty Black Ops 1. Game is probably already closed or stuck at launch in Steam. Try killing the game from Steam or Task Manager.");
    uiControlEnable(uiControl(launchButton));
    return 0;
}

static int onWidgetsWindowClose(uiWindow *window, void *data) {
    (void)window;
    (void)data;
    uiControlHide(uiControl(widgetsWindow));
    return 0;
}

static int onBindsWindowClose(uiWindow *window, void *data) {
    (void)window;
    (void)data;
    if (uiBindsIsSavable()) {
        int okPressed = uiMsgBoxOkCancel(parent, "Are you sure?", "You have pending changes.");
        if (!okPressed) return 0;
        uiBindsReset();
    }
    uiControlHide(uiControl(bindsWindow));
    return 0;
}

// Helpers
static bool isValidExecutableName(char *gamePath) {
    size_t gamePathLen = strlen(gamePath);
    size_t gameExecLen = strlen(GAME_EXECUTABLE_NAME);

    if (gameExecLen > gamePathLen) {
        return false;
    }

    return strcmp(gamePath + gamePathLen - gameExecLen, GAME_EXECUTABLE_NAME) == 0;
}

// Extract directory from full executable path
static void extractDirectory(const char *fullPath, char *dirOut, size_t dirOutSize) {
    strncpy(dirOut, fullPath, dirOutSize - 1);
    dirOut[dirOutSize - 1] = '\0';
    char *lastSlash = strrchr(dirOut, '\\');
    if (!lastSlash) lastSlash = strrchr(dirOut, '/');
    if (lastSlash) *lastSlash = '\0';
}

// Listeners
static void onCheckboxToggled(uiCheckbox *checkbox, void *data) {
    CheatName cheatName = (CheatName)(uintptr_t)data;
    bool enabled = uiCheckboxChecked(checkbox);

    if (clientSetCheat(client, cheatName, enabled) != CLIENT_OK) {
        uiCheckboxSetChecked(checkbox, !enabled);
    } else {
        guiSnapshotSetCheat(cheatName, enabled);
    }
}

static void onHostnameEntryChange(uiEntry *entry, void *data) {
    (void)data;
    (void)entry;
    char *hostname = uiEntryText(hostnameEntry);
    clientSetGameHostname(client, hostname);
    uiFreeText(hostname);
}

static void onLocationEntryChange(uiEntry *entry, void *data) {
    (void)data;
    (void)entry;
    char *location = uiEntryText(locationEntry);
    clientSetGameLocation(client, location);
    uiFreeText(location);
}

static void onChangeRoundSpinChanged(uiSpinbox *spin, void *data) {
    (void)data;
    if (uiSpinboxValue(spin) < 4) {
        uiControlDisable(uiControl(changeRoundButton));
    } else {
        uiControlEnable(uiControl(changeRoundButton));
    }
}

static void onLaunchButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    GameConfigInfo config;
    bool haveLocation = (clientGetGameConfig(client, &config) == CLIENT_OK) && config.location[0] != '\0';
    if (!haveLocation) {
        uiMsgBox(parent, "Launch game", "Couldn't find game location. Only for this time, manually open Call of Duty Black Ops 1 to resolve the executable location.");
        char *gamePath = uiOpenFile(parent);
        if (!gamePath) {
            return;
        } else if (!isValidExecutableName(gamePath)) {
            uiFreeText(gamePath);
            uiMsgBoxError(parent, "Launch game", "This seems to not be Black Ops 1 executable! Make sure to select the correct executable \"BlackOps.exe\".");
            return;
        }
        // Store only the directory, not the full executable path
        char gameDir[MAX_PATH];
        extractDirectory(gamePath, gameDir, MAX_PATH);
        uiEntrySetText(locationEntry, gameDir);
        clientSetGameLocation(client, gameDir);
        uiFreeText(gamePath);
    }
    // Run a new thread to avoid blocking the UI while game starts.
    Thread *gameLauncherThread = threadCreate(threadLaunchGame, NULL);
    threadCreateWatchdog(gameLauncherThread, 15000, onLaunchGameError, NULL);
}

static void onChangeRoundButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    clientSetRound(client, uiSpinboxValue(changeRoundSpin));
}

static void onBindsButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    uiBindsReset();
    uiWindowSetResizeable(bindsWindow, false);
    uiWindowSetMargined(bindsWindow, true);
    uiWindowSetIcon(bindsWindow, IDI_ICON1);
    uiControlShow(uiControl(bindsWindow));
}

static void onWidgetsButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    uiWidgetsReload();
    uiControlShow(uiControl(widgetsWindow));
}

static void onCamoButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    uiCamoShow(client, parent);
}

static void onTwitchButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    uiTwitchShow(client, parent);
}

static void onGscButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    uiGscShow(client);
}

static void onLogsButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    uiLogsShow();
}

static void onCloseButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    // Run a new thread to avoid blocking the UI while game closes.
    Thread *gameCloserThread = threadCreate(threadCloseGame, NULL);
    threadCreateWatchdog(gameCloserThread, 15000, onCloseGameError, NULL);
}


// Builders
static void buildWidgets() {
    widgetsWindow = uiNewWindow("Widget Settings", 50, 200, 0);
    uiControl *widgetsGroup = uiWidgetsBuild(client, widgetsWindow);
    
    uiWindowOnClosing(widgetsWindow, onWidgetsWindowClose, NULL);
    uiWindowSetMargined(widgetsWindow, 1);
    uiWindowSetChild(widgetsWindow, uiControl(widgetsGroup));
    uiWindowSetResizeable(widgetsWindow, false);
    uiWindowSetMargined(widgetsWindow, true);
    uiWindowSetIcon(widgetsWindow, IDI_ICON1);
}

static void buildBinds() {
    bindsWindow = uiNewWindow("Bind Manager", 800, 350, 0);
    uiControl *bindsGroup = uiBindsBuild(client, bindsWindow);
    
    uiWindowOnClosing(bindsWindow, onBindsWindowClose, NULL);
    uiWindowSetMargined(bindsWindow, 1);
    uiWindowSetChild(bindsWindow, uiControl(bindsGroup));
}

static void init() {
    GameConfigInfo config;
    if (clientGetGameConfig(client, &config) == CLIENT_OK) {
        uiEntrySetText(hostnameEntry, config.hostname);
        uiEntrySetText(locationEntry, config.location);
    }
}

static uiControl *build(Client *clientInstance, uiWindow *parentInstance) {
    client = clientInstance;
    parent = parentInstance;

    uiGroup *gameGroup = uiNewGroup("Game");
    uiBox *gameBox = uiNewVerticalBox();
    uiBoxSetPadded(gameBox, 1);

    uiGrid *grid = uiNewGrid();
    uiGridSetPadded(grid, 1);
    hostnameLabel = uiNewLabel("Hostname");
    hostnameEntry = uiNewEntry();
    changeRoundButton = uiNewButton("Change Round");
    changeRoundSpin = uiNewSpinbox(0, 255);
    patchMovementCheckbox = uiNewCheckbox(" Fix Movement Speed");
    showFpsCheckbox = uiNewCheckbox(" Show FPS");
    bindsButton = uiNewButton("Bind Keys");
    widgetsButton = uiNewButton("Add Widgets");
    camoButton = uiNewButton("Camo Manager");
    twitchButton = uiNewButton("Twitch Integration");
    gscButton = uiNewButton("GSC Mods");
    logsButton = uiNewButton("Logs");
    launchButton = uiNewButton("Launch Game");
    closeButton = uiNewButton("Close Game");
    locationEntry = uiNewEntry();

    uiControlDisable(uiControl(closeButton));
    uiControlDisable(uiControl(changeRoundButton));
    uiControlHide(uiControl(locationEntry));

    uiEntrySetPlaceholder(hostnameEntry, "I am below scoreboard!");

    uiCheckboxOnToggled(patchMovementCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_FIX_MOVEMENT_SPEED);
    uiCheckboxOnToggled(showFpsCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_SHOW_FPS);

    uiEntryOnChanged(hostnameEntry, onHostnameEntryChange, NULL);
    uiEntryOnChanged(locationEntry, onLocationEntryChange, NULL);

    uiSpinboxOnChanged(changeRoundSpin, onChangeRoundSpinChanged, NULL);

    uiButtonOnClicked(changeRoundButton, onChangeRoundButtonClick, NULL);
    uiButtonOnClicked(bindsButton, onBindsButtonClick, NULL);
    uiButtonOnClicked(widgetsButton, onWidgetsButtonClick, NULL);
    uiButtonOnClicked(camoButton, onCamoButtonClick, NULL);
    uiButtonOnClicked(twitchButton, onTwitchButtonClick, NULL);
    uiButtonOnClicked(gscButton, onGscButtonClick, NULL);
    uiButtonOnClicked(logsButton, onLogsButtonClick, NULL);
    uiButtonOnClicked(launchButton, onLaunchButtonClick, NULL);
    uiButtonOnClicked(closeButton, onCloseButtonClick, NULL);

    uiGridAppend(grid, uiControl(hostnameLabel),            0, 0, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(grid, uiControl(hostnameEntry),            1, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(patchMovementCheckbox),    0, 1, 2, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(showFpsCheckbox),          1, 1, 2, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(changeRoundButton),        0, 2, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(changeRoundSpin),          1, 2, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(bindsButton),              0, 3, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(widgetsButton),            1, 3, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(camoButton),               0, 4, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(twitchButton),             1, 4, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(gscButton),                0, 5, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(logsButton),               1, 5, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(launchButton),             0, 6, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(closeButton),              1, 6, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    uiBoxAppend(gameBox, uiControl(grid), 1);

    uiGroupSetChild(gameGroup, uiControl(gameBox));
    uiGroupSetMargined(gameGroup, 1);

    init();
    buildWidgets();
    buildBinds();

    return uiControl(gameGroup);
}


static void update() {
    const GuiSnapshot *s = guiGetSnapshot();

    // fix-movement-speed / show-fps checkboxes reflect live cheat state.
    bool enabled;
    if (guiSnapshotCheat(CHEAT_NAME_FIX_MOVEMENT_SPEED, &enabled) &&
        uiCheckboxChecked(patchMovementCheckbox) != enabled) {
        uiCheckboxSetChecked(patchMovementCheckbox, enabled);
    }
    if (guiSnapshotCheat(CHEAT_NAME_SHOW_FPS, &enabled) &&
        uiCheckboxChecked(showFpsCheckbox) != enabled) {
        uiCheckboxSetChecked(showFpsCheckbox, enabled);
    }

    // Sync hostname entry with the configured value.
    if (s->gameConfigValid) {
        char *currentHostname = uiEntryText(hostnameEntry);
        if (strcmp(currentHostname, s->gameConfig.hostname) != 0) {
            uiEntrySetText(hostnameEntry, s->gameConfig.hostname);
        }
        uiFreeText(currentHostname);
    }

    bool attached = s->statusValid ? s->status.attached : gameAttached;
    // Avoid modifying components constantly
    if (attached != gameAttached) {
        gameAttached = attached;
        if (attached) {
            uiControlDisable(uiControl(launchButton));
            uiControlEnable(uiControl(closeButton));
        } else {
            uiControlEnable(uiControl(launchButton));
            uiControlDisable(uiControl(closeButton));
        }
    }
    uiWidgetsUpdate();
    uiBindsUpdate();
}

bool uiGamePromptLocation(char *outDir, size_t size) {
    if (!outDir || size == 0) return false;
    int okPressed = uiMsgBoxOkCancel(parent, "Welcome to Black Ops 1 Zombies Trainer!", 
             "Before training the be next Black Ops 1 Zombies hero...\nI need to know where your game is installed.\nPlease select the BlackOps.exe executable to continue.");

    if (!okPressed) return false;

    char *gamePath = uiOpenFile(parent);
    if (!gamePath) return false;

    while (!isValidExecutableName(gamePath)) {
        uiFreeText(gamePath);
        uiMsgBoxError(parent, "Invalid Selection", 
                    "This doesn't appear to be BlackOps.exe. Please select the correct executable.");   
        gamePath = uiOpenFile(parent);
        if (!gamePath) return false;
    }
    
    // Store only the directory, not the full executable path
    char gameDir[MAX_PATH];
    extractDirectory(gamePath, gameDir, MAX_PATH);
    uiFreeText(gamePath);
    if (locationEntry) uiEntrySetText(locationEntry, gameDir);
    snprintf(outDir, size, "%s", gameDir);
    return true;
}

UIControlGroup *uiGameBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}
