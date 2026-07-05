#include "gui/teleport.h"
#include "client/player.h"
#include "logic/cheat.h"
#include <stdio.h>
#include <stdlib.h>

// Shared HTTP client
static Client *client;

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
    clientTeleport(client, (float)uiSpinboxValue(xSpin), (float)uiSpinboxValue(ySpin), (float)uiSpinboxValue(zSpin));
}

static void onTeleportSaveButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    TeleportCoords coords;
    if (clientGetPosition(client, &coords) != CLIENT_OK) {
        uiMsgBoxError(parent, "Error", "You should be on a Zombies Game to save your current coords!");
        return;
    }
    char *filePath = uiSaveFile(parent);
    if (filePath == NULL) return;

    FILE *fp = fopen(filePath, "w");
    if (fp == NULL) {
        uiMsgBoxError(parent, "Unexpected error", "Error saving the coords");
        uiFreeText(filePath);
        return;
    }

    fprintf(fp, "%d %d %d", (int)coords.x, (int)coords.y, (int)coords.z);
    fclose(fp);
    uiFreeText(filePath);
}

static void onTeleportLoadButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    char *filePath = uiOpenFile(parent);
    if (!filePath) return;
    
    int x, y, z;
    FILE *fp = fopen(filePath, "r");
    if (fp == NULL || fscanf(fp, "%d %d %d", &x, &y, &z) != 3) {
        uiMsgBoxError(parent, "Unexpected error", "Error loading the coords");
        uiFreeText(filePath);
        return;
    }

    uiSpinboxSetValue(xSpin, x);
    uiSpinboxSetValue(ySpin, y);
    uiSpinboxSetValue(zSpin, z);    
    uiFreeText(filePath);
}

static uiControl *build(Client *clientInstance, uiWindow *parentInstance) {
    client = clientInstance;
    parent = parentInstance;

    uiGroup *teleportGroup = uiNewGroup("Teleport");
    uiBox *teleportBox = uiNewVerticalBox();
    uiBoxSetPadded(teleportBox, 1);

    uiGrid *teleportGrid = uiNewGrid();
    uiGridSetPadded(teleportGrid, 1);

    xSpin = uiNewSpinbox(-500000, 500000);
    ySpin = uiNewSpinbox(-500000, 500000);
    zSpin = uiNewSpinbox(-500000, 500000);


    goBtn = uiNewButton("Go");
    uiButton *loadBtn = uiNewButton("Load Position");
    uiButton *saveBtn = uiNewButton("Save Position");

    uiButtonOnClicked(goBtn, onTeleportGoButtonClick, NULL);
    uiButtonOnClicked(saveBtn, onTeleportSaveButtonClick, NULL);
    uiButtonOnClicked(loadBtn, onTeleportLoadButtonClick, NULL);
    
    uiSpinboxSetValue(xSpin, 0);
    uiSpinboxSetValue(ySpin, 0);
    uiSpinboxSetValue(zSpin, 0);
    
    uiGridAppend(teleportGrid, uiControl(xSpin),        0, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(teleportGrid, uiControl(ySpin),        0, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(teleportGrid, uiControl(zSpin),        0, 2, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(teleportGrid, uiControl(loadBtn),      1, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(teleportGrid, uiControl(saveBtn),      1, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(teleportGrid, uiControl(goBtn),        1, 2, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    uiBoxAppend(teleportBox, uiControl(teleportGrid), 1);

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
