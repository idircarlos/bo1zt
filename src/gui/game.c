#include "gui/game.h"
#include "gui.h"
#include "gui/widgets.h"
#include "gui/binds.h"
#include "logger.h"
#include "logic/state.h"
#include "logic/cheat/manager.h"
#include "logic/cheat/manager/actions.h"
#include "win/thread.h"
#include "utils/map.h"
#include "resource_ids.h"
#include <windows.h>
#include <stdbool.h>
#include <string.h>
#include <ui.h>
#include <stdio.h>
#include <stdlib.h>

#define RUNNING_TEXT "Running"
#define NOT_RUNNING_TEXT "Not running"
#define OPENING_TEXT "Opening"
#define CLOSING_TEXT "Closing"

#define CACHE_GAME_ATTACHED "GAME_ATTACHED"
#define CACHE_TIM_RUNNING "TIM_RUNNING"
#define CACHE_RESETS "RESETS"
#define CACHE_OPENING_GAME "OPENING_GAME"
#define CACHE_CLOSING_GAME "CLOSING_GAME"

static Map *cache = NULL;

// Controller instance
static Controller *controller;
static uiWindow *parent;

// --- UI Elements ---
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

static uiArea *timArea = NULL;
static uiAreaHandler timHandler;
static uiAttributedString *timCurrentText = NULL;
static uiAttributedString *timNotRunningText = NULL;
static uiAttributedString *timRunningText = NULL;

static uiLabel *statusLabel = NULL;
static uiLabel *resetsLabel = NULL;
static uiLabel *resetsNumLabel = NULL;
static uiLabel *timLabel = NULL;
static uiCheckbox *patchMovementCheckbox = NULL;
static uiCheckbox *showFpsCheckbox = NULL;
static uiLabel *hostnameLabel = NULL;
static uiEntry *hostnameEntry = NULL;
static uiButton *bindsButton = NULL;
static uiButton *widgetsButton = NULL;
static uiButton *launchButton = NULL;
static uiButton *closeButton = NULL;

static uiEntry *locationEntry = NULL; // Using this conponent to act as another UI Component but it's purpose is only to update ConfigGame. This componente will be always hidden and not have any parent.

// Widgets window
static uiWindow *widgetsWindow = NULL;
static UIControlGroup *widgetsControlGroup = NULL;

// Binds window
static uiWindow *bindsWindow = NULL;
static UIControlGroup *bindsControlGroup = NULL;

// Threads
static int threadLaunchGame(void *data) {
    (void)data;
    LOG_INFO("Launching game from UI\n");
    uiControlDisable(uiControl(launchButton));
    mapPutBool(cache, CACHE_OPENING_GAME, true);
    bool success = controllerLaunchGame(controller);
    mapPutBool(cache, CACHE_OPENING_GAME, false);
    if (!success) {
        uiMsgBoxError(parent, "Launch game", "Couldn't launch Call of Duty Black Ops 1. Location may have changed or the game is already running.");
        return 1;
    }
    return 0;
}

static int threadCloseGame(void *data) {
    (void)data;
    LOG_INFO("Closing game from UI\n");
    uiControlDisable(uiControl(closeButton));
    mapPutBool(cache, CACHE_CLOSING_GAME, true);
    bool success = controllerCloseGame(controller);
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
    if (uiWidgetsIsSavable()) {
        int okPressed = uiMsgBoxOkCancel(parent, "Are you sure?", "You have pending changes.");
        if (!okPressed) return 0;
        uiWidgetsReset();
    }
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

// Aux
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

static void handlerTimDraw(uiAreaHandler *a, uiArea *area, uiAreaDrawParams *p) {
    (void)a;
    (void)area;
    if (!timCurrentText)
        return;

    uiFontDescriptor font;
    uiDrawTextLayoutParams params;
    uiDrawTextLayout *layout;

    uiLoadControlFont(&font);
    params.String = timCurrentText;
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
    
    CheatManager *cheatManager = controllerGetCheatManager(controller);
    if (!cheatManager) {
        // Fallback: update Config directly if CheatManager not available
        Config *config = controllerGetConfig(controller);
        switch (cheatName) {
            case CHEAT_NAME_FIX_MOVEMENT_SPEED:
                config->game.fixMovementSpeed = enabled;
                break;
            case CHEAT_NAME_SHOW_FPS:
                config->game.showFps = enabled;
                break;
            default:
                break;
        }
        configSave(config);
        return;
    }
    
    CheatResult result = cheatManagerSetToggle(cheatManager, cheatName, enabled);
    
    // If API failed, revert checkbox to previous state
    if (result == CHEAT_RESULT_API_FAILED) {
        uiCheckboxSetChecked(checkbox, !enabled);
    }
}

static void onEntryChange(uiEntry *entry, void *data) {
    (void)data;
    Config *config = controllerGetConfig(controller);
    
    if (entry == hostnameEntry) {
        char *hostname = uiEntryText(hostnameEntry);
        strncpy(config->game.hostname, hostname, sizeof(config->game.hostname) - 1);
        uiFreeText(hostname);
    } else if (entry == locationEntry) {
        char *location = uiEntryText(locationEntry);
        strncpy(config->game.location, location, sizeof(config->game.location) - 1);
        uiFreeText(location);
    }
    configSave(config);
}

static void onLaunchButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    GameConfig config = controllerGetGameConfig(controller);
    if (strcmp(config.location, "") == 0) {
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
        Config *config = controllerGetConfig(controller);
        strncpy(config->game.location, gameDir, sizeof(config->game.location) - 1);
        configSave(config);
        uiFreeText(gamePath);
    }
    // Run a new thread to avoid blocking the UI while game starts.
    Thread *gameLauncherThread = threadCreate(threadLaunchGame, NULL);
    threadCreateWatchdog(gameLauncherThread, 15000, onLaunchGameError, NULL);
}

static void onBindsButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    uiWindowSetResizeable(bindsWindow, false);
    uiWindowSetMargined(bindsWindow, true);
    uiWindowSetIcon(bindsWindow, IDI_ICON1);
    uiControlShow(uiControl(bindsWindow));
}

static void onWidgetsButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    uiWindowSetResizeable(widgetsWindow, false);
    uiWindowSetMargined(widgetsWindow, true);
    uiWindowSetIcon(widgetsWindow, IDI_ICON1);
    uiControlShow(uiControl(widgetsWindow));
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
    widgetsControlGroup = uiWidgetsBuildControlGroup();
    
    widgetsWindow = uiNewWindow("Widget Settings", 50, 200, 0);
    uiControl *widgetsGroup = widgetsControlGroup->build(controller, widgetsWindow);
    
    uiWindowOnClosing(widgetsWindow, onWidgetsWindowClose, NULL);
    uiWindowSetMargined(widgetsWindow, 1);
    uiWindowSetChild(widgetsWindow, uiControl(widgetsGroup));
}

static void buildBinds() {
    bindsControlGroup = uiBindsBuildControlGroup();
    
    bindsWindow = uiNewWindow("Bind Manager", 800, 350, 0);
    uiControl *bindsGroup = bindsControlGroup->build(controller, bindsWindow);
    
    uiWindowOnClosing(bindsWindow, onBindsWindowClose, NULL);
    uiWindowSetMargined(bindsWindow, 1);
    uiWindowSetChild(bindsWindow, uiControl(bindsGroup));
}

static void init() {
    cache = mapCreate();
    mapPutBool(cache, CACHE_GAME_ATTACHED, false);
    mapPutBool(cache, CACHE_TIM_RUNNING, false);
    mapPutInt(cache, CACHE_RESETS, 0);
    mapPutBool(cache, CACHE_OPENING_GAME, false);
    mapPutBool(cache, CACHE_CLOSING_GAME, false);
    GameConfig config = controllerGetGameConfig(controller);
    uiCheckboxSetChecked(patchMovementCheckbox, config.fixMovementSpeed);
    uiCheckboxSetChecked(showFpsCheckbox, config.showFps);
    uiEntrySetText(hostnameEntry, config.hostname);
    uiEntrySetText(locationEntry, config.location);
}

static uiControl *build(Controller *controllerInstance, uiWindow *parentInstance) {
    controller = controllerInstance;
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
    timNotRunningText = buildInfoAttributedString(NOT_RUNNING_TEXT, attrRed, &timHandler, handlerTimDraw);
    timRunningText = buildInfoAttributedString(RUNNING_TEXT, attrGreen, &timHandler, handlerTimDraw);
    timCurrentText = timNotRunningText;

    // --- Game Group ---
    uiGroup *gameGroup = uiNewGroup("Game");
    uiBox *gameBox = uiNewVerticalBox();
    uiBoxSetPadded(gameBox, 1);

    uiGrid *grid = uiNewGrid();
    uiGridSetPadded(grid, 1);
    statusLabel = uiNewLabel("Status");
    statusArea = uiNewArea(&statusHandler);
    timLabel = uiNewLabel("TIM");
    timArea = uiNewArea(&timHandler);
    resetsLabel = uiNewLabel("Resets");
    resetsNumLabel = uiNewLabel("0");
    hostnameLabel = uiNewLabel("Hostname");
    hostnameEntry = uiNewEntry();
    patchMovementCheckbox = uiNewCheckbox(" Fix Movement Speed");
    showFpsCheckbox = uiNewCheckbox(" Show FPS");
    bindsButton = uiNewButton("Bind Keys");
    widgetsButton = uiNewButton("Add Widgets");
    launchButton = uiNewButton("Launch Game");
    closeButton = uiNewButton("Close Game");
    locationEntry = uiNewEntry();

    uiControlDisable(uiControl(closeButton));
    uiControlHide(uiControl(locationEntry));

    uiCheckboxOnToggled(patchMovementCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_FIX_MOVEMENT_SPEED);
    uiCheckboxOnToggled(showFpsCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_SHOW_FPS);

    uiEntryOnChanged(hostnameEntry, onEntryChange, NULL);

    uiButtonOnClicked(bindsButton, onBindsButtonClick, NULL);
    uiButtonOnClicked(widgetsButton, onWidgetsButtonClick, NULL);
    uiButtonOnClicked(launchButton, onLaunchButtonClick, NULL);
    uiButtonOnClicked(closeButton, onCloseButtonClick, NULL);

    uiGridAppend(grid, uiControl(statusLabel),              0, 0, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(statusArea),               1, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(timLabel),                 0, 1, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(timArea),                  1, 1, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(resetsLabel),              0, 2, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(resetsNumLabel),           1, 2, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(hostnameLabel),            0, 3, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(grid, uiControl(hostnameEntry),            1, 3, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(patchMovementCheckbox),    0, 4, 2, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(showFpsCheckbox),          1, 4, 2, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(bindsButton),              0, 5, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(widgetsButton),            1, 5, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
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
    Config *config = controllerGetConfig(controller);
    GameConfig *game = &config->game;
    
    // Sync UI with config values (in case commands changed them)
    if (uiCheckboxChecked(patchMovementCheckbox) != game->fixMovementSpeed) {
        uiCheckboxSetChecked(patchMovementCheckbox, game->fixMovementSpeed);
    }
    if (uiCheckboxChecked(showFpsCheckbox) != game->showFps) {
        uiCheckboxSetChecked(showFpsCheckbox, game->showFps);
    }
    
    State *state = controllerGetState(controller);
    bool gameAttached = state->isGameAttached;
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
    bool timRunning = state->isTimRunning;
    if (timRunning != mapGetBool(cache, CACHE_TIM_RUNNING)) {
        timCurrentText = timRunning ? timRunningText : timNotRunningText;
        uiAreaQueueRedrawAll(timArea);
        mapPutBool(cache, CACHE_TIM_RUNNING, timRunning);
        if (timRunning) {
            uiControlDisable(uiControl(hostnameLabel));
            uiControlDisable(uiControl(hostnameEntry));
        } else {
            uiControlEnable(uiControl(hostnameLabel));
            uiControlEnable(uiControl(hostnameEntry));
        }
    }
    int resets = state->gameResets;
    if (resets != mapGetInt(cache, CACHE_RESETS)) {
        char resetsStr[4];
        sprintf(resetsStr, "%d", resets);
        uiLabelSetText(resetsNumLabel, resetsStr);
        mapPutInt(cache, CACHE_RESETS, resets);
    }
    widgetsControlGroup->update();
    bindsControlGroup->update();
}

// External API for Controller
bool uiGameIsChecked(CheatName cheat) {
    switch (cheat) {
        case CHEAT_NAME_FIX_MOVEMENT_SPEED:
            return uiCheckboxChecked(patchMovementCheckbox);
        case CHEAT_NAME_SHOW_FPS:
            return uiCheckboxChecked(showFpsCheckbox);
        default:
            fprintf(stderr, "Unknown cheat %d\n", cheat);
            return false;
    }
}

char *uiGameGetLocation() {
    return uiEntryText(locationEntry);
}

char *uiGameGetHostname() {
    return uiEntryText(hostnameEntry);
}

void uiGameSetLocation(const char *location) {
    if (locationEntry && location) {
        uiEntrySetText(locationEntry, location);
    }
}

bool uiGamePromptLocation(void) {    
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
    uiGameSetLocation(gameDir);
    uiFreeText(gamePath);
    Config *config = controllerGetConfig(controller);
    strncpy(config->game.location, gameDir, sizeof(config->game.location) - 1);
    configSave(config);
    return true;
}

UIControlGroup *uiGameBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}
