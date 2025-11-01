#include "game.h"
#include "../../logger/logger.h"
#include <string.h>
#include <ui.h>
#include <stdio.h>

// Controller instance
static Controller *controller;
static uiWindow *parent;

// --- UI Elements ---
static uiArea *statusArea = NULL;
static uiAreaHandler statusHandler;
static uiAttributedString *statusText = NULL;

static uiArea *timArea = NULL;
static uiAreaHandler timHandler;
static uiAttributedString *timText = NULL;

static uiLabel *statusLabel = NULL;
static uiLabel *resetsLabel = NULL;
static uiLabel *resetsNumLabel = NULL;
static uiLabel *timLabel = NULL;
static uiCheckbox *patchMovementCheckbox = NULL;
static uiCheckbox *showFpsCheckbox = NULL;
static uiButton *hostnameButton = NULL;
static uiEntry *hostnameEntry = NULL;
static uiButton *camosButton = NULL;
static uiButton *widgetsButton = NULL;
static uiButton *launchButton = NULL;
static uiButton *closeButton = NULL;

// ----------------------------------------------------------------------
// Draw "Not running" in red + bold
// ----------------------------------------------------------------------

static void makeStatusAttributedString(void) {
    statusText = uiNewAttributedString("Not running");

    uiAttribute *attrRed = uiNewColorAttribute(1.0, 0.0, 0.0, 1.0);
    uiAttribute *attrBold = uiNewWeightAttribute(uiTextWeightBold);

    size_t len = uiAttributedStringLen(statusText);
    uiAttributedStringSetAttribute(statusText, attrRed, 0, len);
    uiAttributedStringSetAttribute(statusText, attrBold, 0, len);
}

static void makeTimAttributedString(void) {
    timText = uiNewAttributedString("Not running");

    uiAttribute *attrRed = uiNewColorAttribute(1.0, 0.0, 0.0, 1.0);
    uiAttribute *attrBold = uiNewWeightAttribute(uiTextWeightBold);

    size_t len = uiAttributedStringLen(timText);
    uiAttributedStringSetAttribute(timText, attrRed, 0, len);
    uiAttributedStringSetAttribute(timText, attrBold, 0, len);
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
    if (!statusText)
        return;

    uiFontDescriptor font;
    uiDrawTextLayoutParams params;
    uiDrawTextLayout *layout;

    uiLoadControlFont(&font);
    params.String = statusText;
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
    if (!timText)
        return;

    uiFontDescriptor font;
    uiDrawTextLayoutParams params;
    uiDrawTextLayout *layout;

    uiLoadControlFont(&font);
    params.String = timText;
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
    bool success = controllerIsGameRunning(controller) ? controllerSetCheat(controller, cheatName, enabled) : true; // Allowing modifying checkboxes if the game is not running since they will be updated as soon as it starts.
    if (!success) {
        fprintf(stderr, "Failed to set Game cheat %d to %d\n", cheatName, enabled);
        uiCheckboxSetChecked(checkbox, !enabled); // Revert checkbox state
    }
}

static uiGroup *build(Controller *controllerInstance, uiWindow *parentInstance) {
    controller = controllerInstance;
    parent = parentInstance;

    makeStatusAttributedString();
    makeTimAttributedString();

    memset(&statusHandler, 0, sizeof(uiAreaHandler));
    statusHandler.Draw = handlerStatusDraw;
    statusHandler.DragBroken = handlerUnusedDragBroken;
    statusHandler.KeyEvent = handlerUnusedKeyEvent;
    statusHandler.MouseCrossed = handlerUnusedMouseCrossed;
    statusHandler.MouseEvent = handlerUnusedMouseEvent;

    memset(&timHandler, 0, sizeof(uiAreaHandler));
    timHandler.Draw = handlerTimDraw;
    timHandler.DragBroken = handlerUnusedDragBroken;
    timHandler.KeyEvent = handlerUnusedKeyEvent;
    timHandler.MouseCrossed = handlerUnusedMouseCrossed;
    timHandler.MouseEvent = handlerUnusedMouseEvent;

    // --- Game Group ---
    uiGroup *gameGroup = uiNewGroup("Game");
    uiBox *gameBox = uiNewVerticalBox();
    uiBoxSetPadded(gameBox, 1);

    uiGrid *grid = uiNewGrid();
    uiGridSetPadded(grid, 1);

    statusLabel = uiNewLabel("Status:");
    statusArea = uiNewArea(&statusHandler);
    timLabel = uiNewLabel("TIM:");
    timArea = uiNewArea(&timHandler);
    resetsLabel = uiNewLabel("Resets:");
    resetsNumLabel = uiNewLabel("0");
    patchMovementCheckbox = uiNewCheckbox(" Fix Movement Speed");
    showFpsCheckbox = uiNewCheckbox(" Show FPS");
    hostnameButton = uiNewButton("Set Hostname");
    hostnameEntry = uiNewEntry();
    camosButton = uiNewButton("Setup Camos");
    widgetsButton = uiNewButton("Add Widgets");
    launchButton = uiNewButton("Launch Game");
    closeButton = uiNewButton("Close Game");

    uiCheckboxOnToggled(patchMovementCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_FIX_MOVEMENT_SPEED);
    uiCheckboxOnToggled(showFpsCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_SHOW_FPS);

    uiGridAppend(grid, uiControl(statusLabel),              0, 0, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(statusArea),               1, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(timLabel),                 0, 1, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(timArea),                  1, 1, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(resetsLabel),              0, 2, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(resetsNumLabel),           1, 2, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(patchMovementCheckbox),    0, 3, 2, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(showFpsCheckbox),          1, 3, 2, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(hostnameButton),           0, 4, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(hostnameEntry),            1, 4, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(camosButton),              0, 5, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(widgetsButton),            1, 5, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(launchButton),             0, 6, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, uiControl(closeButton),              1, 6, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    uiBoxAppend(gameBox, uiControl(grid), 1);

    uiGroupSetChild(gameGroup, uiControl(gameBox));
    uiGroupSetMargined(gameGroup, 1);

    return gameGroup;
}

static void update() {
    // Nothing for now
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

UIControlGroup *uiGameBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}
