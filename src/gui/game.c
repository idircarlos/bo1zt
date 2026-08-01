#include "gui/game.h"
#include "gui.h"
#include "gui/widgets.h"
#include "gui/binds.h"
#include "gui/camo.h"
#include "client/cheats.h"
#include "client/game.h"
#include "client/round.h"
#include "logger.h"
#include "logic/cheat.h"
#include "win/thread.h"
#include "utils/map.h"
#include "resource_ids.h"
#include <windows.h>
#include <stdbool.h>
#include <string.h>
#include <ui.h>
#include <stdio.h>
#include <stdlib.h>

#define GAME_EXECUTABLE_NAME "BlackOps.exe"

#define RUNNING_TEXT "Running"
#define NOT_RUNNING_TEXT "Not running"
#define OPENING_TEXT "Opening"
#define CLOSING_TEXT "Closing"

#define CACHE_GAME_ATTACHED "GAME_ATTACHED"
#define CACHE_OPENING_GAME "OPENING_GAME"
#define CACHE_CLOSING_GAME "CLOSING_GAME"

static Map *cache = NULL;

// Shared HTTP client
static Client *client;
static uiWindow *parent;

// UI Elements
static uiAttribute *attrRed = NULL;
static uiAttribute *attrGreen = NULL;
static uiAttribute *attrBlue = NULL;
static uiAttribute *attrOrange = NULL;
static uiAttribute *attrBold = NULL;

static uiArea *statusArea = NULL;
static uiAreaHandler statusHandler;
static uiAttributedString *statusCurrentText = NULL;
static uiAttributedString *statusNotRunningText = NULL;
static uiAttributedString *statusRunningText = NULL;
static uiAttributedString *statusOpeningText = NULL;
static uiAttributedString *statusClosingText = NULL;

static uiLabel *statusLabel = NULL;
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
static uiButton *launchButton = NULL;
static uiButton *closeButton = NULL;

static uiEntry *locationEntry = NULL;

static uiWindow *widgetsWindow = NULL;

static uiWindow *bindsWindow = NULL;

static int threadLaunchGame(void *data) {
    (void)data;
    LOG_INFO("Launching game from UI");
    uiControlDisable(uiControl(launchButton));
    mapPutBool(cache, CACHE_OPENING_GAME, true);
    Client *localClient = clientCreate(guiClientPort());
    bool success = localClient && clientLaunchGame(localClient) == CLIENT_OK;
    clientDestroy(localClient);
    mapPutBool(cache, CACHE_OPENING_GAME, false);
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
    mapPutBool(cache, CACHE_CLOSING_GAME, true);
    Client *localClient = clientCreate(guiClientPort());
    bool success = localClient && clientCloseGame(localClient) == CLIENT_OK;
    clientDestroy(localClient);
    mapPutBool(cache, CACHE_CLOSING_GAME, false);
    if (!success) {
        uiMsgBoxError(parent, "Close game", "Couldn't close Call of Duty Black Ops 1. Game is probably already closed.");
        return 1;
    }
    return 0; 
}

static int onLaunchGameError(void *data) {
    (void)data;
    mapPutBool(cache, CACHE_OPENING_GAME, false);
    uiMsgBoxError(parent, "Launch game", "Couldn't launch Call of Duty Black Ops 1. Probably is either running or stuck at launch in Steam. Try killing the game from Steam or Task Manager.");
    uiControlEnable(uiControl(launchButton));
    return 0;
}

static int onCloseGameError(void *data) {
    (void)data;
    mapPutBool(cache, CACHE_CLOSING_GAME, false);
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
static void handlerUnusedDragBroken(uiAreaHandler *a, uiArea *area) {
    (void)a;
    (void)area;
}

static int handlerUnusedKeyEvent(uiAreaHandler *a, uiArea *area, uiAreaKeyEvent *e) {
    (void)a;
    (void)area;
    (void)e;
    return 0;
}

static void handlerUnusedMouseCrossed(uiAreaHandler *a, uiArea *area, int left) {
    (void)a;
    (void)area;
    (void)left;
}

static void handlerUnusedMouseEvent(uiAreaHandler *a, uiArea *area, uiAreaMouseEvent *e) {
    (void)a;
    (void)area;
    (void)e;
}

static void handlerStatusDraw(uiAreaHandler *a, uiArea *area, uiAreaDrawParams *p) {
    (void)a;
    (void)area;
    if (!statusCurrentText)
        return;
    uiFontDescriptor font;
    uiDrawTextLayoutParams params;
    uiDrawTextLayout *layout;

    uiLoadControlFont(&font);
    params.String = statusCurrentText;
    params.DefaultFont = &font;
    params.Width = p->AreaWidth;
    params.Align = uiDrawTextAlignLeft;

    layout = uiDrawNewTextLayout(&params);
    uiDrawText(p->Context, layout, 0, -0.5);
    uiDrawFreeTextLayout(layout);

    uiFreeFontButtonFont(&font);
}

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
    // TODO: open Twitch Integration window
}

static void onCloseButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    // Run a new thread to avoid blocking the UI while game closes.
    Thread *gameCloserThread = threadCreate(threadCloseGame, NULL);
    threadCreateWatchdog(gameCloserThread, 15000, onCloseGameError, NULL);
}


// Builders
static uiAttributedString *buildInfoAttributedString(const char *str, uiAttribute *colorAttribute, uiAreaHandler *areaHandler, void (*handlerDraw)(uiAreaHandler *, uiArea *, uiAreaDrawParams *)) {
    memset(areaHandler, 0, sizeof(uiAreaHandler));
    areaHandler->Draw = handlerDraw;
    areaHandler->DragBroken = handlerUnusedDragBroken;
    areaHandler->KeyEvent = handlerUnusedKeyEvent;
    areaHandler->MouseCrossed = handlerUnusedMouseCrossed;
    areaHandler->MouseEvent = handlerUnusedMouseEvent;

    uiAttributedString *attributedString = uiNewAttributedString(str);
    size_t len = uiAttributedStringLen(attributedString);
    uiAttributedStringSetAttribute(attributedString, colorAttribute, 0, len);
    uiAttributedStringSetAttribute(attributedString, attrBold, 0, len);
    return attributedString;
}

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
    cache = mapCreate();
    mapPutBool(cache, CACHE_GAME_ATTACHED, false);
    mapPutBool(cache, CACHE_OPENING_GAME, false);
    mapPutBool(cache, CACHE_CLOSING_GAME, false);
    GameConfigInfo config;
    if (clientGetGameConfig(client, &config) == CLIENT_OK) {
        uiEntrySetText(hostnameEntry, config.hostname);
        uiEntrySetText(locationEntry, config.location);
    }
}

static uiControl *build(Client *clientInstance, uiWindow *parentInstance) {
    client = clientInstance;
    parent = parentInstance;

    attrRed = uiNewColorAttribute(181/256.0, 38/256.0, 62/256.0, 1.0);
    attrGreen = uiNewColorAttribute(38/256.0, 181/256.0, 90/256.0, 1.0);
    attrBlue = uiNewColorAttribute(52/256.0, 88/256.0, 235/256.0, 1.0);
    attrOrange = uiNewColorAttribute(232/256.0, 142/256.0, 39/256.0, 1.0);
    attrBold = uiNewWeightAttribute(uiTextWeightBold);

    statusNotRunningText = buildInfoAttributedString(NOT_RUNNING_TEXT, attrRed, &statusHandler, handlerStatusDraw);
    statusRunningText = buildInfoAttributedString(RUNNING_TEXT, attrGreen, &statusHandler, handlerStatusDraw);
    statusOpeningText = buildInfoAttributedString(OPENING_TEXT, attrBlue, &statusHandler, handlerStatusDraw);
    statusClosingText = buildInfoAttributedString(CLOSING_TEXT, attrOrange, &statusHandler, handlerStatusDraw);
    statusCurrentText = statusNotRunningText;

    uiGroup *gameGroup = uiNewGroup("Game");
    uiBox *gameBox = uiNewVerticalBox();
    uiBoxSetPadded(gameBox, 1);

    uiGrid *grid = uiNewGrid();
    uiGridSetPadded(grid, 1);
    statusLabel = uiNewLabel("Status");
    statusArea = uiNewArea(&statusHandler);
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
    uiButtonOnClicked(launchButton, onLaunchButtonClick, NULL);
    uiButtonOnClicked(closeButton, onCloseButtonClick, NULL);

    uiGridAppend(grid, uiControl(statusLabel),              0, 0, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(statusArea),               1, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(hostnameLabel),            0, 1, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(grid, uiControl(hostnameEntry),            1, 1, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(patchMovementCheckbox),    0, 2, 2, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(showFpsCheckbox),          1, 2, 2, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(changeRoundButton),        0, 3, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(changeRoundSpin),          1, 3, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(bindsButton),              0, 4, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(widgetsButton),            1, 4, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(camoButton),               0, 5, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(twitchButton),             1, 5, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
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

    bool gameAttached = s->statusValid ? s->status.attached : mapGetBool(cache, CACHE_GAME_ATTACHED);
    // Avoid redrawing the area and modifying components constantly
    if (gameAttached != mapGetBool(cache, CACHE_GAME_ATTACHED)) {
        statusCurrentText = gameAttached ? statusRunningText : statusNotRunningText;
        uiAreaQueueRedrawAll(statusArea);
        mapPutBool(cache, CACHE_GAME_ATTACHED, gameAttached);
        if (gameAttached == false) {
            uiControlEnable(uiControl(launchButton));
            uiControlDisable(uiControl(closeButton));
        } else {
            uiControlDisable(uiControl(launchButton));
            uiControlEnable(uiControl(closeButton));
        }
    } else if (mapGetBool(cache, CACHE_OPENING_GAME)) {
        statusCurrentText = statusOpeningText;
        uiAreaQueueRedrawAll(statusArea);
    } else if (mapGetBool(cache, CACHE_CLOSING_GAME)) {
        statusCurrentText = statusClosingText;
        uiAreaQueueRedrawAll(statusArea);
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
