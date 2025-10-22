#include "graphics.h"
#include "../../logger/logger.h"

// Controller instance
static Controller *controller;

// Parent Window instance
static uiWindow *parent;

// Components
static uiSpinbox *fovSpin = NULL;
static uiSpinbox *fovScaleSpin = NULL;
static uiSpinbox *fpsSpin = NULL;
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

static uiGroup *build(Controller *controllerInstance, uiWindow *parentInstance) {
    controller = controllerInstance;
    parent = parentInstance;
    // --- Graphics Group ---
    uiGroup *graphicsGroup = uiNewGroup("Graphics");
    uiBox *graphicsBox = uiNewVerticalBox();
    uiBoxSetPadded(graphicsBox, 1);

    fovLabel = uiNewLabel("FOV ");
    fovScaleLabel = uiNewLabel("FOV Scale");
    fpsCapLabel = uiNewLabel("FPS Cap");

    fovSpin = uiNewSpinbox(10, 160);
    fovScaleSpin = uiNewSpinbox(0, 100);
    fpsSpin = uiNewSpinbox(60, 1000);

    unlimitFpsCheckbox = uiNewCheckbox(" Unlimit FPS");
    makeBorderlessCheckbox = uiNewCheckbox(" Make Borderless");
    disableHudCheckbox = uiNewCheckbox(" Disable HUD");
    fogCheckbox = uiNewCheckbox(" Fog");
    fullbrightCheckbox = uiNewCheckbox(" Fullbright");
    colorizedCheckbox = uiNewCheckbox(" Colorized");

    customizeUiButton = uiNewButton("Customize UI");

    uiGrid *graphicsGrid = uiNewGrid();
    uiGridSetPadded(graphicsGrid, 1);
    uiGridAppend(graphicsGrid, uiControl(fovLabel), 0, 0, 1, 1, 0, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(graphicsGrid, uiControl(fovSpin), 1, 0, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(graphicsGrid, uiControl(fovScaleLabel), 0, 1, 1, 1, 0, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(graphicsGrid, uiControl(fovScaleSpin), 1, 1, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(graphicsGrid, uiControl(fpsCapLabel), 0, 2, 1, 1, 0, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(graphicsGrid, uiControl(fpsSpin), 1, 2, 1, 1, 0, uiAlignFill, 0, uiAlignCenter);
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
    return graphicsGroup;
}

static void update() {
    // Nothing
}

UIControlGroup *uiGraphicsBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}
