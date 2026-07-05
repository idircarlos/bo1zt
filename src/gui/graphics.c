#include "gui/graphics.h"
#include "gui/customizer.h"
#include "client/graphics.h"
#include "logger.h"
#include "logic/cheat.h"
#include "resource_ids.h"

// Shared HTTP client
static Client *client;

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
static UIControlGroup *customizerControlGroup = NULL;

static GraphicsConfig readGraphicsUi(void) {
    GraphicsConfig g;
    g.fov        = uiSpinboxValue(fovSpin);
    g.fovScale   = uiSpinboxValue(fovScaleSpin);
    g.fpsCap     = uiSpinboxValue(fpsCapSpin);
    g.borderless = uiCheckboxChecked(makeBorderlessCheckbox);
    g.unlimitFps = uiCheckboxChecked(unlimitFpsCheckbox);
    g.disableHud = uiCheckboxChecked(disableHudCheckbox);
    g.disableFog = uiCheckboxChecked(fogCheckbox);
    g.fullbright = uiCheckboxChecked(fullbrightCheckbox);
    g.colorized  = uiCheckboxChecked(colorizedCheckbox);
    return g;
}

static void setFpsCapEnabled(bool enabled) {
    if (enabled) {
        uiControlEnable(uiControl(fpsCapLabel));
        uiEnableSpinbox(fpsCapSpin);
    } else {
        uiControlDisable(uiControl(fpsCapLabel));
        uiDisableSpinbox(fpsCapSpin);
    }
}

// Handlers
static void onSpinboxChange(uiSpinbox *spin, void *data) {
    (void)spin;
    (void)data;
    GraphicsConfig g = readGraphicsUi();
    clientSetGraphics(client, &g);
    guiPollNow();
}

static void onCheckboxToggled(uiCheckbox *checkbox, void *data) {
    CheatName cheatName = (CheatName)(uintptr_t)data;
    bool enabled = uiCheckboxChecked(checkbox);

    GraphicsConfig g = readGraphicsUi();
    if (clientSetGraphics(client, &g) != CLIENT_OK) {
        uiCheckboxSetChecked(checkbox, !enabled);
    } else {
        guiPollNow();
    }

    if (cheatName == CHEAT_NAME_UNLIMIT_FPS) {
        setFpsCapEnabled(!enabled);
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
    customizerControlGroup = uiCustomizerBuildControlGroup();

    customizerWindow = uiNewWindow("Customize UI", 200, 200, 0);
    uiControl *customizerGroup = customizerControlGroup->build(client, customizerWindow);

    uiWindowOnClosing(customizerWindow, onCustomizerClose, NULL);
    uiWindowSetMargined(customizerWindow, 1);
    uiWindowSetChild(customizerWindow, uiControl(customizerGroup));
}

static void init() {
    GraphicsConfig g;
    if (clientGetGraphics(client, &g) != CLIENT_OK) return;
    uiSpinboxSetValue(fovSpin, g.fov);
    uiSpinboxSetValue(fovScaleSpin, g.fovScale);
    uiSpinboxSetValue(fpsCapSpin, g.fpsCap);
    uiCheckboxSetChecked(makeBorderlessCheckbox, g.borderless);
    uiCheckboxSetChecked(unlimitFpsCheckbox, g.unlimitFps);
    uiCheckboxSetChecked(disableHudCheckbox, g.disableHud);
    uiCheckboxSetChecked(fogCheckbox, g.disableFog);
    uiCheckboxSetChecked(fullbrightCheckbox, g.fullbright);
    uiCheckboxSetChecked(colorizedCheckbox, g.colorized);
    setFpsCapEnabled(!g.unlimitFps);
}

static uiControl *build(Client *clientInstance, uiWindow *parentInstance) {
    client = clientInstance;
    parent = parentInstance;

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
    const GuiSnapshot *s = guiGetSnapshot();
    if (!s->graphicsValid) return;
    const GraphicsConfig *g = &s->graphics;

    if (uiSpinboxValue(fovSpin) != g->fov) {
        uiSpinboxSetValue(fovSpin, g->fov);
    }
    if (uiSpinboxValue(fovScaleSpin) != g->fovScale) {
        uiSpinboxSetValue(fovScaleSpin, g->fovScale);
    }
    if (uiSpinboxValue(fpsCapSpin) != g->fpsCap) {
        uiSpinboxSetValue(fpsCapSpin, g->fpsCap);
    }
    if (uiCheckboxChecked(makeBorderlessCheckbox) != g->borderless) {
        uiCheckboxSetChecked(makeBorderlessCheckbox, g->borderless);
    }
    if (uiCheckboxChecked(unlimitFpsCheckbox) != g->unlimitFps) {
        uiCheckboxSetChecked(unlimitFpsCheckbox, g->unlimitFps);
        setFpsCapEnabled(!g->unlimitFps);
    }
    if (uiCheckboxChecked(disableHudCheckbox) != g->disableHud) {
        uiCheckboxSetChecked(disableHudCheckbox, g->disableHud);
    }
    if (uiCheckboxChecked(fogCheckbox) != g->disableFog) {
        uiCheckboxSetChecked(fogCheckbox, g->disableFog);
    }
    if (uiCheckboxChecked(fullbrightCheckbox) != g->fullbright) {
        uiCheckboxSetChecked(fullbrightCheckbox, g->fullbright);
    }
    if (uiCheckboxChecked(colorizedCheckbox) != g->colorized) {
        uiCheckboxSetChecked(colorizedCheckbox, g->colorized);
    }
}

UIControlGroup *uiGraphicsBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}
