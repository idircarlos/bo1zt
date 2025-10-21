#include "game.h"
#include "../../logger/logger.h"

// Controller instance
static Controller *controller;

// Parent Window instance
static uiWindow *parent;


static uiGroup *build(Controller *controllerInstance, uiWindow *parentInstance) {
    controller = controllerInstance;
    parent = parentInstance;
    // --- Game Group ---
    uiGroup *gameGroup = uiNewGroup("Game");
    uiBox *gameBox = uiNewVerticalBox();
    uiBoxSetPadded(gameBox, 1);
    return gameGroup;
}

static void update() {
    // Nothing
}

UIControlGroup *uiGameBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}
