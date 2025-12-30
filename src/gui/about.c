#include "gui/about.h"

// Controller instance
static Controller *controller;

// Parent Window instance
static uiWindow *parent;

static uiControl *build(Controller *controllerInstance, uiWindow *parentInstance) {
    controller = controllerInstance;
    parent = parentInstance;

    uiGroup *aboutGroup = uiNewGroup("About");
    uiBox *aboutBox = uiNewVerticalBox();
    uiBoxSetPadded(aboutBox, 1);

    uiLabel *placeholderLabel = uiNewLabel("About section placeholder");
    uiBoxAppend(aboutBox, uiControl(placeholderLabel), 0);

    uiGroupSetChild(aboutGroup, uiControl(aboutBox));
    uiGroupSetMargined(aboutGroup, 1);
    return uiControl(aboutGroup);
}

static void update() {
    // Nothing
}

UIControlGroup *uiAboutBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}
