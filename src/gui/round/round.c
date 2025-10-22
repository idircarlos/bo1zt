#include "round.h"
#include "../../logger/logger.h"

// Controller instance
static Controller *controller;

// Parent Window instance
static uiWindow *parent;

// Change Round
static uiSpinbox *currentSpin = NULL;
static uiSpinbox *nextSpin = NULL;
static uiButton *changeRoundBtn = NULL;

static void onChangeRoundButtonClicked(uiButton *button, void *data) {
    (void)button;
    (void)data;
    int currentRound = uiSpinboxValue(currentSpin);
    int nextRound = uiSpinboxValue(nextSpin);
    controllerSetRound(controller, currentRound, nextRound);
}

static uiGroup *build(Controller *controllerInstance, uiWindow *parentInstance) {
    controller = controllerInstance;
    parent = parentInstance;
    // --- Round Changer Group ---
    uiGroup *roundGroup = uiNewGroup("Round Changer");
    uiBox *roundBox = uiNewVerticalBox();
    uiBoxSetPadded(roundBox, 1);

    uiGrid *roundGrid = uiNewGrid();
    uiGridSetPadded(roundGrid, 1);
    uiLabel *currentLabel = uiNewLabel("Current Round");
    currentSpin = uiNewSpinbox(0, 256);
    uiLabel *nextLabel = uiNewLabel("Next Round");
    nextSpin = uiNewSpinbox(0, 256);

    uiLabel *infoLabel = uiNewLabel(
        "Wait until Round 2 before using this.\nE.g: Current round 2, Next round 19.\nPress Change Round.\nComplete your current round."
    );

    changeRoundBtn = uiNewButton("Change Round");
    uiButtonOnClicked(changeRoundBtn, onChangeRoundButtonClicked, NULL);

    uiGridAppend(roundGrid, uiControl(currentLabel), 0, 0, 1, 1, 0, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(roundGrid, uiControl(currentSpin), 1, 0, 1, 1, 0, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(roundGrid, uiControl(nextLabel), 0, 1, 1, 1, 0, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(roundGrid, uiControl(nextSpin), 1, 1, 1, 1, 0, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(roundGrid, uiControl(infoLabel), 0, 2, 2, 1, 1, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(roundGrid, uiControl(changeRoundBtn), 0, 3, 2, 1, 1, uiAlignFill, 1, uiAlignFill);

    uiBoxAppend(roundBox, uiControl(roundGrid), 1);

    uiGroupSetChild(roundGroup, uiControl(roundBox));
    uiGroupSetMargined(roundGroup, 1);
    return roundGroup;
}

static void update() {
    // Nothing
}

UIControlGroup *uiRoundBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}
