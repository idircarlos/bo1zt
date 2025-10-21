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
    uiBox *roundVBox = uiNewVerticalBox();
    uiBoxSetPadded(roundVBox, 1);

    uiGrid *roundGrid = uiNewGrid();
    uiGridSetPadded(roundGrid, 1);
    uiLabel *currentLabel = uiNewLabel("Current Round");
    currentSpin = uiNewSpinbox(0, 256);
    uiLabel *nextLabel = uiNewLabel("Next Round");
    nextSpin = uiNewSpinbox(0, 256);
    uiGridAppend(roundGrid, uiControl(currentLabel), 0, 0, 1, 1, 0, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(roundGrid, uiControl(currentSpin), 1, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(roundGrid, uiControl(nextLabel), 0, 1, 1, 1, 0, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(roundGrid, uiControl(nextSpin), 1, 1, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiBoxAppend(roundVBox, uiControl(roundGrid), 0);

    uiLabel *infoLabel = uiNewLabel(
        "Wait until Round 2 before using this.\nE.g: Current round 2, Next round 19, then press Change Round.\nComplete your current round and the next will be 20."
    );
    uiBoxAppend(roundVBox, uiControl(infoLabel), 1);

    changeRoundBtn = uiNewButton("Change Round");
    uiButtonOnClicked(changeRoundBtn, onChangeRoundButtonClicked, NULL);

    uiBoxAppend(roundVBox, uiControl(changeRoundBtn), 1);

    uiGroupSetChild(roundGroup, uiControl(roundVBox));
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
