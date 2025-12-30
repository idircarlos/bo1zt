#include "gui/character.h"

// Controller instance
static Controller *controller;

// Parent Window instance
static uiWindow *parent;

static uiControl *build(Controller *controllerInstance, uiWindow *parentInstance) {
    controller = controllerInstance;
    parent = parentInstance;

    uiGroup *characterGroup = uiNewGroup("Character");
    uiBox *characterBox = uiNewVerticalBox();
    uiBoxSetPadded(characterBox, 1);

    uiLabel *placeholderLabel = uiNewLabel("Character selector placeholder");
    uiBoxAppend(characterBox, uiControl(placeholderLabel), 0);

    uiGroupSetChild(characterGroup, uiControl(characterBox));
    uiGroupSetMargined(characterGroup, 1);
    return uiControl(characterGroup);
}

static void update() {
    // Nothing
}

UIControlGroup *uiCharacterBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}
