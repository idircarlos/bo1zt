#include "gui/graphics.h"
#include "gui/customizer.h"
#include "logic/cheat/manager.h"
#include "logic/cheat/manager/actions.h"
#include "utils/map.h"
#include "resource_ids.h"
#include <stdio.h>

#define UI_CUSTOMIZER_CONTROL_GROUP_SIZE 1

#define CACHE_TIM_RUNNING "TIM_RUNNING"

static Map *cache = NULL;

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

static uiWindow *customizerWindow = NULL;
static UIControlGroup *controlGroups[] = {NULL};

// Handlers
static void onSpinboxChange(uiSpinbox *spin, void *data) {
    SimpleCheatName simpleCheatName = (SimpleCheatName)(uintptr_t)data;
    int value = uiSpinboxValue(spin);
    
    CheatManager *cheatManager = controllerGetCheatManager(controller);
    cheatManagerSetValue(cheatManager, simpleCheatName, &value);
}

static void onCheckboxToggled(uiCheckbox *checkbox, void *data) {
    CheatName cheatName = (CheatName)(uintptr_t)data;
    bool enabled = uiCheckboxChecked(checkbox);
    
    CheatManager *cheatManager = controllerGetCheatManager(controller);    
    CheatResult result = cheatManagerSetToggle(cheatManager, cheatName, enabled);
    
    // If API failed, revert checkbox to previous state
    if (result == CHEAT_RESULT_API_FAILED) {
        uiCheckboxSetChecked(checkbox, !enabled);
    }
    
    // Handle unlimitFps UI state change
    if (cheatName == CHEAT_NAME_UNLIMIT_FPS) {
        if (enabled) {
            uiControlDisable(uiControl(fpsCapLabel));
            uiDisableSpinbox(fpsCapSpin);
        } else {
            uiControlEnable(uiControl(fpsCapLabel));
            uiEnableSpinbox(fpsCapSpin);
        }
    }
}

static int onCustomizerClose(uiWindow *window, void *data) {
    (void)window;
    (void)data;
    if (uiCustomizerIsSavable()) {
        int okPressed = uiMsgBoxOkCancel(parent, "Are you sure?", "You have pending changes.");
        if (!okPressed) return 0;
        uiCustomizerReset();
    }
    uiControlHide(uiControl(customizerWindow));
    return 0;
}

static void onButtonCustomizeUiClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    uiWindowSetResizeable(customizerWindow, false);
    uiWindowSetMargined(customizerWindow, true);
    uiWindowSetIcon(customizerWindow, IDI_ICON1);
    uiControlShow(uiControl(customizerWindow));
}

static void buildCustomizer() {
    UIControlGroup *customizerControlGroup = uiCustomizerBuildControlGroup();
    controlGroups[0] = customizerControlGroup;

    customizerWindow = uiNewWindow("Customize UI", 200, 200, 0);    
    uiControl *customizerGroup = customizerControlGroup->build(controller, customizerWindow);

    uiWindowOnClosing(customizerWindow, onCustomizerClose, NULL);
    uiWindowSetMargined(customizerWindow, 1);
    uiWindowSetChild(customizerWindow, uiControl(customizerGroup));
}

static void init() {
    cache = mapCreate();
    mapPutBool(cache, CACHE_TIM_RUNNING, false);
    GraphicsConfig config = controllerGetGraphicsConfig(controller);
    uiSpinboxSetValue(fovSpin, config.fov);
    uiSpinboxSetValue(fovScaleSpin, config.fovScale);
    uiSpinboxSetValue(fpsCapSpin, config.fpsCap);
    uiCheckboxSetChecked(makeBorderlessCheckbox, config.borderless);
    uiCheckboxSetChecked(unlimitFpsCheckbox, config.unlimitFps);
    uiCheckboxSetChecked(disableHudCheckbox, config.disableHud);
    uiCheckboxSetChecked(fogCheckbox, config.disableFog);
    uiCheckboxSetChecked(fullbrightCheckbox, config.fullbright);
    uiCheckboxSetChecked(colorizedCheckbox, config.colorized);
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
    buildCustomizer();

    uiCheckboxOnToggled(makeBorderlessCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_MAKE_BORDERLESS);
    uiCheckboxOnToggled(unlimitFpsCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_UNLIMIT_FPS);
    uiCheckboxOnToggled(disableHudCheckbox, onCheckboxToggled, (void*) CHEAT_NAME_DISABLE_HUD);
    uiCheckboxOnToggled(fogCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_DISABLE_FOG);
    uiCheckboxOnToggled(fullbrightCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_FULLBRIGHT);
    uiCheckboxOnToggled(colorizedCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_COLORIZED);

    uiSpinboxOnChanged(fovSpin, onSpinboxChange, (void*)SIMPLE_CHEAT_NAME_FOV);
    uiSpinboxOnChanged(fovScaleSpin, onSpinboxChange, (void*)SIMPLE_CHEAT_NAME_FOV_SCALE);
    uiSpinboxOnChanged(fpsCapSpin, onSpinboxChange, (void*)SIMPLE_CHEAT_NAME_FPS_CAP);

    uiButtonOnClicked(customizeUiButton, onButtonCustomizeUiClick, NULL);

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

    init();

    return uiControl(graphicsGroup);
}


static void update() {
    Config *config = controllerGetConfig(controller);
    GraphicsConfig *graphics = &config->graphics;
    
    // Sync UI with config values (in case commands changed them)
    if (uiSpinboxValue(fovSpin) != graphics->fov) {
        uiSpinboxSetValue(fovSpin, graphics->fov);
    }
    if (uiSpinboxValue(fovScaleSpin) != graphics->fovScale) {
        uiSpinboxSetValue(fovScaleSpin, graphics->fovScale);
    }
    if (uiSpinboxValue(fpsCapSpin) != graphics->fpsCap) {
        uiSpinboxSetValue(fpsCapSpin, graphics->fpsCap);
    }
    if (uiCheckboxChecked(makeBorderlessCheckbox) != graphics->borderless) {
        uiCheckboxSetChecked(makeBorderlessCheckbox, graphics->borderless);
    }
    if (uiCheckboxChecked(unlimitFpsCheckbox) != graphics->unlimitFps) {
        uiCheckboxSetChecked(unlimitFpsCheckbox, graphics->unlimitFps);
    }
    if (uiCheckboxChecked(disableHudCheckbox) != graphics->disableHud) {
        uiCheckboxSetChecked(disableHudCheckbox, graphics->disableHud);
    }
    if (uiCheckboxChecked(fogCheckbox) != graphics->disableFog) {
        uiCheckboxSetChecked(fogCheckbox, graphics->disableFog);
    }
    if (uiCheckboxChecked(fullbrightCheckbox) != graphics->fullbright) {
        uiCheckboxSetChecked(fullbrightCheckbox, graphics->fullbright);
    }
    if (uiCheckboxChecked(colorizedCheckbox) != graphics->colorized) {
        uiCheckboxSetChecked(colorizedCheckbox, graphics->colorized);
    }
    
    // Handle TIM running state
    State *state = controllerGetState(controller);
    bool timRunning = state->isTimRunning;
    if (timRunning != mapGetBool(cache, CACHE_TIM_RUNNING)) {
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
        mapPutBool(cache, CACHE_TIM_RUNNING, timRunning);
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
