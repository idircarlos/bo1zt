#include "graphics.h"
#include "../../logger/logger.h"
#include <stdio.h>

static bool cachedTimRunning = false;

// Controller instance
static Controller *controller;

// Parent Window instance
static uiWindow *parent;

// Components
static uiSpinbox *fovSpin = NULL;
static uiSpinbox *fovScaleSpin = NULL;
static uiSpinbox *fpsCapSpin = NULL;
static uiLabel *fovLabel = NULL;
static uiLabel *fovScaleLabel = NULL;
static uiLabel *fpsCapLabel = NULL;
static uiButton *customizeUiButton = NULL;
static uiCheckbox *makeBorderlessCheckbox = NULL;
static uiCheckbox *unlimitFpsCheckbox = NULL;
static uiCheckbox *disableHudCheckbox = NULL;
static uiCheckbox *fogCheckbox = NULL;
static uiCheckbox *fullbrightCheckbox = NULL;
static uiCheckbox *colorizedCheckbox = NULL;

// Handlers
static void onSpinboxChange(uiSpinbox *spin, void *data) {
    (void)spin;
    (void)data;
    SimpleCheatName simpleCheatName = (SimpleCheatName)(uintptr_t)data;
    int value = (float)uiSpinboxValue(spin);
    controllerSetSimpleCheat(controller, simpleCheatName, &value);
}

static void onCheckboxToggled(uiCheckbox *checkbox, void *data) {
    CheatName cheatName = (CheatName)(uintptr_t)data;
    bool enabled = uiCheckboxChecked(checkbox);
    bool success = controllerIsGameAttached(controller) ? controllerSetCheat(controller, cheatName, enabled) : true; // Allowing modifying checkboxes if the game is not running since they will be updated as soon as it starts.
    if (!success) {
        fprintf(stderr, "Failed to set Graphics cheat %d to %d\n", cheatName, enabled);
        uiCheckboxSetChecked(checkbox, !enabled); // Revert checkbox state
    }
    // If "Unlimit FPS" checkbox is toggled, enable/disable FPS Cap Spinbox
    if (cheatName == CHEAT_NAME_UNLIMIT_FPS && success) {
        if (enabled) {
            uiControlDisable(uiControl(fpsCapLabel));
            uiDisableSpinbox(fpsCapSpin);
        } else {
            uiControlEnable(uiControl(fpsCapLabel));
            uiEnableSpinbox(fpsCapSpin);
        }
    }
}


static uiControl *build(Controller *controllerInstance, uiWindow *parentInstance) {
    controller = controllerInstance;
    parent = parentInstance;
    // --- Graphics Group ---
    uiGroup *graphicsGroup = uiNewGroup("Graphics");
    uiBox *graphicsBox = uiNewVerticalBox();
    uiBoxSetPadded(graphicsBox, 1);

    fovLabel = uiNewLabel("FOV");
    fovScaleLabel = uiNewLabel("FOV Scale %");
    fpsCapLabel = uiNewLabel("FPS Cap");

    fovSpin = uiNewSpinbox(10, 160);
    fovScaleSpin = uiNewSpinbox(20, 200);
    fpsCapSpin = uiNewSpinbox(60, 1000);

    unlimitFpsCheckbox = uiNewCheckbox(" Unlimit FPS");
    makeBorderlessCheckbox = uiNewCheckbox(" Make Borderless");
    disableHudCheckbox = uiNewCheckbox(" Disable HUD");
    fogCheckbox = uiNewCheckbox(" Disable Fog");
    fullbrightCheckbox = uiNewCheckbox(" Fullbright");
    colorizedCheckbox = uiNewCheckbox(" Colorized");

    customizeUiButton = uiNewButton("Customize UI");

    uiSpinboxSetValue(fovSpin, 90);
    uiSpinboxSetValue(fovScaleSpin, 100);
    uiSpinboxSetValue(fpsCapSpin, 165);

    uiCheckboxOnToggled(makeBorderlessCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_MAKE_BORDERLESS);
    uiCheckboxOnToggled(unlimitFpsCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_UNLIMIT_FPS);
    uiCheckboxOnToggled(disableHudCheckbox, onCheckboxToggled, (void*) CHEAT_NAME_DISABLE_HUD);
    uiCheckboxOnToggled(fogCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_DISABLE_FOG);
    uiCheckboxOnToggled(fullbrightCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_FULLBRIGHT);
    uiCheckboxOnToggled(colorizedCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_COLORIZED);

    uiSpinboxOnChanged(fovSpin, onSpinboxChange, (void*)SIMPLE_CHEAT_NAME_FOV);
    uiSpinboxOnChanged(fovScaleSpin, onSpinboxChange, (void*)SIMPLE_CHEAT_NAME_FOV_SCALE);
    uiSpinboxOnChanged(fpsCapSpin, onSpinboxChange, (void*)SIMPLE_CHEAT_NAME_FPS_CAP);

    uiGrid *graphicsGrid = uiNewGrid();
    uiGridSetPadded(graphicsGrid, 1);
    uiGridAppend(graphicsGrid, uiControl(fovLabel), 0, 0, 1, 1, 0, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(graphicsGrid, uiControl(fovSpin), 1, 0, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(graphicsGrid, uiControl(fovScaleLabel), 0, 1, 1, 1, 0, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(graphicsGrid, uiControl(fovScaleSpin), 1, 1, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(graphicsGrid, uiControl(fpsCapLabel), 0, 2, 1, 1, 0, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(graphicsGrid, uiControl(fpsCapSpin), 1, 2, 1, 1, 0, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(graphicsGrid, uiControl(makeBorderlessCheckbox), 0, 3, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(graphicsGrid, uiControl(unlimitFpsCheckbox), 1, 3, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(graphicsGrid, uiControl(disableHudCheckbox), 0, 4, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(graphicsGrid, uiControl(fogCheckbox), 1, 4, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(graphicsGrid, uiControl(fullbrightCheckbox), 0, 5, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(graphicsGrid, uiControl(colorizedCheckbox), 1, 5, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(graphicsGrid, uiControl(customizeUiButton), 0, 6, 2, 1, 1, uiAlignFill, 1, uiAlignFill);

    uiBoxAppend(graphicsBox, uiControl(graphicsGrid), 1);

    uiGroupSetChild(graphicsGroup, uiControl(graphicsBox));
    uiGroupSetMargined(graphicsGroup, 1);
    return uiControl(graphicsGroup);
}

static void update() {
    State *state = controllerGetState(controller);
    bool timRunning = stateIsTimRunning(state);
    if (timRunning != cachedTimRunning) {
        if (timRunning) {
            uiDisableSpinbox(fovSpin);
            uiDisableSpinbox(fovScaleSpin);
            uiDisableSpinbox(fpsCapSpin);
            uiControlDisable(uiControl(fovLabel));
            uiControlDisable(uiControl(fovScaleLabel));
            uiControlDisable(uiControl(fpsCapLabel));
            uiControlDisable(uiControl(unlimitFpsCheckbox));
        } else {
            uiEnableSpinbox(fovSpin);
            uiEnableSpinbox(fovScaleSpin);
            uiEnableSpinbox(fpsCapSpin);
            uiControlEnable(uiControl(fovLabel));
            uiControlEnable(uiControl(fovScaleLabel));
            uiControlEnable(uiControl(fpsCapLabel));
            uiControlEnable(uiControl(unlimitFpsCheckbox));
        }
        cachedTimRunning = timRunning;
    }
}

UIControlGroup *uiGraphicsBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}

// External API for Controller
bool uiGraphicsIsChecked(CheatName cheat) {
    switch (cheat) {
        case CHEAT_NAME_MAKE_BORDERLESS:
            return uiCheckboxChecked(makeBorderlessCheckbox);
        case CHEAT_NAME_UNLIMIT_FPS:
            return uiCheckboxChecked(unlimitFpsCheckbox);
        case CHEAT_NAME_DISABLE_HUD:
            return uiCheckboxChecked(disableHudCheckbox);
        case CHEAT_NAME_DISABLE_FOG:
            return uiCheckboxChecked(fogCheckbox);
        case CHEAT_NAME_FULLBRIGHT:
            return uiCheckboxChecked(fullbrightCheckbox);
        case CHEAT_NAME_COLORIZED:
            return uiCheckboxChecked(colorizedCheckbox);
        default:
            fprintf(stderr, "Unknown cheat %d\n", cheat);
            return false;
    }
}

int uiGraphicsGetFov() {
    return uiSpinboxValue(fovSpin);
}

int uiGraphicsGetFovScale() {
    return uiSpinboxValue(fovScaleSpin);
}

int uiGraphicsGetFpsCap() {
    return uiSpinboxValue(fpsCapSpin);
}