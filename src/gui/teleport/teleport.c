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

static uiGroup *build(Controller *controllerInstance, uiWindow *parentInstance) {
    controller = controllerInstance;
    parent = parentInstance;
    // --- Teleport Group ---
    uiGroup *teleportGroup = uiNewGroup("Teleport");
    uiBox *teleportHBox = uiNewHorizontalBox();
    uiBoxSetPadded(teleportHBox, 1);

    // --- Grid de coordenadas (3x2: labels + spinboxes)
    uiBox *coordsVBox = uiNewVerticalBox();
    uiBoxSetPadded(coordsVBox, 1);

    xSpin = uiNewSpinbox(-500000, 500000);
    ySpin = uiNewSpinbox(-500000, 500000);
    zSpin = uiNewSpinbox(-500000, 500000);

    // Añadir etiquetas y spinboxes en el grid (3 filas, 2 columnas)
    uiBoxAppend(coordsVBox, uiControl(xSpin), 1);
    uiBoxAppend(coordsVBox, uiControl(ySpin), 1);
    uiBoxAppend(coordsVBox, uiControl(zSpin), 1);

    // --- Botón Go (a la derecha del grid)
    goBtn = uiNewButton("Go");
    uiButtonOnClicked(goBtn, onTeleportGoButtonClick, NULL);

    // --- VBox con botones Load y Save (a la derecha del botón Go)
    uiBox *saveLoadVBox = uiNewVerticalBox();
    uiBoxSetPadded(saveLoadVBox, 1);
    uiButton *loadBtn = uiNewButton("Load Position");
    uiButton *saveBtn = uiNewButton("Save Position");
    uiBoxAppend(saveLoadVBox, uiControl(loadBtn), 1);
    uiBoxAppend(saveLoadVBox, uiControl(saveBtn), 1);

    uiButtonOnClicked(saveBtn, onTeleportSaveButtonClick, NULL);
    uiButtonOnClicked(loadBtn, onTeleportLoadButtonClick, NULL);
    

    // --- Añadir todo al box principal (coordsVBox + Go + VBox derecha)
    uiBoxAppend(teleportHBox, uiControl(saveLoadVBox), 1);
    uiBoxAppend(teleportHBox, uiControl(coordsVBox), 1);
    uiBoxAppend(teleportHBox, uiControl(goBtn), 1);
    

    // --- Añadir al grupo ---
    uiGroupSetChild(teleportGroup, uiControl(teleportHBox));
    uiGroupSetMargined(teleportGroup, 1);
    return teleportGroup;
}

static void update() {
    // Nothing
}

UIControlGroup *uiTeleportBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}
