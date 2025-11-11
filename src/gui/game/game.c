#include "game.h"
#include "../../logger/logger.h"
#include "../../state/state.h"
#include "../../thread/thread.h"
#include "../../../res/resource_ids.h"
#include <string.h>
#include <ui.h>
#include <stdio.h>
#include <stdlib.h>

#define EMPTY_STRING ""

static const char *RUNNING_TEXT = "Running";
static const char *NOT_RUNNING_TEXT = "Not running";
static const char *OPENING_TEXT = "Opening";
static const char *CLOSING_TEXT = "Closing";

static bool cachedGameAttached = false;
static bool cachedTimRunning = false;
static int cachedResets = 0;
static bool openingGame = false;
static bool closingGame = false;

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
static uiButton *camosButton = NULL;
static uiButton *widgetsButton = NULL;
static uiButton *launchButton = NULL;
static uiButton *closeButton = NULL;

static uiEntry *locationEntry = NULL; // Using this conponent to act as another UI Component but it's purpose is only to update ConfigGame. This componente will be always hidden and not have any parent.

// Threads
static int threadLaunchGame(void *data) {
    (void)data;
    LOG_INFO("Launching game from UI\n");
    uiControlDisable(uiControl(launchButton));
    openingGame = true;
    bool success = controllerLaunchGame(controller);
    openingGame = false;
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
    closingGame = true;
    bool success = controllerCloseGame(controller);
    closingGame = false;
    if (!success) {
        uiMsgBoxError(parent, "Close game", "Couldn't close Call of Duty Black Ops 1. Game is probably already closed.");
        return 1;
    }
    return 0; 
}

static int onLaunchGameError(void *data) {
    (void)data;
    openingGame = false;
    uiMsgBoxError(parent, "Launch game", "Couldn't launch Call of Duty Black Ops 1. Probably is either running or stuck at launch in Steam. Try killing the game from Steam or Task Manager.");
    uiControlEnable(uiControl(launchButton));
    return 0;
}

static int onCloseGameError(void *data) {
    (void)data;
    openingGame = false;
    uiMsgBoxError(parent, "Close game", "Couldn't close Call of Duty Black Ops 1. Game is probably already closed or stuck at launch in Steam. Try killing the game from Steam or Task Manager.");
    uiControlEnable(uiControl(launchButton));
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
    bool success = controllerIsGameAttached(controller) ? controllerSetCheat(controller, cheatName, enabled) : true; // Allowing modifying checkboxes if the game is not running since they will be updated as soon as it starts.
    if (!success) {
        fprintf(stderr, "Failed to set Game cheat %d to %d\n", cheatName, enabled);
        uiCheckboxSetChecked(checkbox, !enabled); // Revert checkbox state
    }
    controllerUpdateConfig(controller, CONFIG_GAME);
}

static void onEntryChange(uiEntry *entry, void *data) {
    (void)entry;
    (void)data;
    controllerUpdateConfig(controller, CONFIG_GAME);
}

static void onLaunchButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    GameConfig config = controllerGetGameConfig(controller);
    if (strcmp(config.location, "") == 0) {
        // TODO: Resolve executable location from process running if the game is not in the default Steam path
        uiMsgBox(parent, "Launch game", "Couldn't find game location. Only for this time, manually open Call of Duty Black Ops 1 to resolve the executable location.");
        char *gamePath = uiOpenFile(parent);
        if (!gamePath) {
            return;
        } else if (!isValidExecutableName(gamePath)) {
            uiFreeText(gamePath);
            uiMsgBoxError(parent, "Launch game", "This seems to not be Black Ops 1 executable! Make sure to select the correct executable \"BlackOps.exe\".");
            return;
        }
        uiEntrySetText(locationEntry, gamePath);
        controllerUpdateConfig(controller, CONFIG_GAME);
        uiFreeText(gamePath);
    }
    // Run a new thread to avoid blocking the UI while game starts.
    Thread *gameLauncherThread = threadCreate(threadLaunchGame, NULL);
    threadCreateWatchdog(gameLauncherThread, 15000, onLaunchGameError, NULL);
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

static void init() {
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
    camosButton = uiNewButton("Setup Camos");
    widgetsButton = uiNewButton("Add Widgets");
    launchButton = uiNewButton("Launch Game");
    closeButton = uiNewButton("Close Game");
    locationEntry = uiNewEntry();

    uiControlDisable(uiControl(closeButton));
    uiControlHide(uiControl(locationEntry));

    uiCheckboxOnToggled(patchMovementCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_FIX_MOVEMENT_SPEED);
    uiCheckboxOnToggled(showFpsCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_SHOW_FPS);

    uiEntryOnChanged(hostnameEntry, onEntryChange, NULL);

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
    uiGridAppend(grid, uiControl(camosButton),              0, 5, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(widgetsButton),            1, 5, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(launchButton),             0, 6, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(closeButton),              1, 6, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    uiBoxAppend(gameBox, uiControl(grid), 1);

    uiGroupSetChild(gameGroup, uiControl(gameBox));
    uiGroupSetMargined(gameGroup, 1);
    
    init();

    return uiControl(gameGroup);
}

static void update() {
    State *state = controllerGetState(controller);
    bool gameAttached = stateIsGameAttached(state);
    // Avoid redrawing the area and modifying components constantly
    if (gameAttached != cachedGameAttached) {
        statusCurrentText = gameAttached ? statusRunningText : statusNotRunningText;
        uiAreaQueueRedrawAll(statusArea);
        cachedGameAttached = gameAttached;
        if (gameAttached == false) {
            uiControlEnable(uiControl(launchButton));
            uiControlDisable(uiControl(closeButton));
        } else {
            uiControlDisable(uiControl(launchButton));
            uiControlEnable(uiControl(closeButton));
        }
    } else if (openingGame) {
        statusCurrentText = statusOpeningText;
        uiAreaQueueRedrawAll(statusArea);
    } else if (closingGame) {
        statusCurrentText = statusClosingText;
        uiAreaQueueRedrawAll(statusArea);
    }
    bool timRunning = stateIsTimRunning(state);
    if (timRunning != cachedTimRunning) {
        timCurrentText = timRunning ? timRunningText : timNotRunningText;
        uiAreaQueueRedrawAll(timArea);
        cachedTimRunning = timRunning;
        if (timRunning) {
            uiControlDisable(uiControl(hostnameLabel));
            uiControlDisable(uiControl(hostnameEntry));
        } else {
            uiControlEnable(uiControl(hostnameLabel));
            uiControlEnable(uiControl(hostnameEntry));
        }
    }
    int resets = stateGetGameResets(state);
    if (resets != cachedResets) {
        char resetsStr[8];
        sprintf(resetsStr, "%d", resets);
        uiLabelSetText(resetsNumLabel, resetsStr);
        cachedResets = resets;
    }
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

UIControlGroup *uiGameBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}
