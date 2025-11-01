#include "teleport.h"
#include "../../logger/logger.h"
#include <stdio.h>
#include <stdlib.h>

// Controller instance
static Controller *controller;

// Parent Window instance
static uiWindow *parent;

// Teleport
static uiSpinbox *xSpin = NULL;
static uiSpinbox *ySpin = NULL;
static uiSpinbox *zSpin = NULL;
static uiButton *goBtn = NULL;

static void onTeleportGoButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    TeleportCoords coords = {.x = (float)uiSpinboxValue(xSpin), .y = (float)uiSpinboxValue(ySpin), .z = (float)uiSpinboxValue(zSpin)};
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_TELEPORT, &coords);
}

static void onTeleportSaveButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    TeleportCoords *coords = controllerGetPlayerCurrentCoords(controller);
    char *filePath = uiSaveFile(parent);
    if (filePath == NULL) return;

    FILE *fp = fopen(filePath, "w");
    if (fp == NULL) {
        uiMsgBoxError(parent, "Unexpected error", "Error saving the coords");
    }

    fprintf(fp, "%d %d %d", (int)coords->x, (int)coords->y, (int)coords->z);
    fclose(fp);
    free(coords);
    uiFreeText(filePath);
}

static void onTeleportLoadButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    char *filePath = uiOpenFile(parent);
    if (!filePath) return;

    FILE *fp = fopen(filePath, "r");
    if (fp == NULL) {
        uiMsgBoxError(parent, "Unexpected error", "Error loading the coords");
    }

    int x, y, z;
    if (fscanf(fp, "%d %d %d", &x, &y, &z) != 3) {
        uiMsgBoxError(parent, "Unexpected error", "Error loading the coords");
    }

    uiSpinboxSetValue(xSpin, x);
    uiSpinboxSetValue(ySpin, y);
    uiSpinboxSetValue(zSpin, z);    
    uiFreeText(filePath);
}

static uiControl *build(Controller *controllerInstance, uiWindow *parentInstance) {
    controller = controllerInstance;
    parent = parentInstance;
    // --- Teleport Group ---
    uiGroup *teleportGroup = uiNewGroup("Teleport");
    uiBox *teleportBox = uiNewVerticalBox();
    uiBoxSetPadded(teleportBox, 1);

    uiGrid *teleportGrid = uiNewGrid();
    uiGridSetPadded(teleportGrid, 1);

    xSpin = uiNewSpinbox(-500000, 500000);
    ySpin = uiNewSpinbox(-500000, 500000);
    zSpin = uiNewSpinbox(-500000, 500000);


    // --- Botón Go (a la derecha del grid)
    goBtn = uiNewButton("Go");
    uiButtonOnClicked(goBtn, onTeleportGoButtonClick, NULL);
    uiButton *loadBtn = uiNewButton("Load Position");
    uiButton *saveBtn = uiNewButton("Save Position");
    uiButtonOnClicked(saveBtn, onTeleportSaveButtonClick, NULL);
    uiButtonOnClicked(loadBtn, onTeleportLoadButtonClick, NULL);
    
    
    uiGridAppend(teleportGrid, uiControl(xSpin),        0, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(teleportGrid, uiControl(ySpin),        0, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(teleportGrid, uiControl(zSpin),        0, 2, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(teleportGrid, uiControl(loadBtn),      1, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(teleportGrid, uiControl(saveBtn),      1, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(teleportGrid, uiControl(goBtn),        1, 2, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    uiBoxAppend(teleportBox, uiControl(teleportGrid), 1);

    // --- Añadir al grupo ---
    uiGroupSetChild(teleportGroup, uiControl(teleportBox));
    uiGroupSetMargined(teleportGroup, 1);
    return uiControl(teleportGroup);
}

static void update() {
    // Nothing
}

UIControlGroup *uiTeleportBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}
