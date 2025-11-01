#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ui.h>
#include "gui.h"
#include "../logger/logger.h"
#include "player/player.h"
#include "round/round.h"
#include "weapons/weapons.h"
#include "teleport/teleport.h"
#include "hacks/hacks.h"
#include "graphics/graphics.h"
#include "game/game.h"

#define WINDOW_WIDTH 100
#define WINDOW_HEIGHT 540
#define UI_CONTROL_GROUP_SIZE 7


// UIControlGroup
static UIControlGroup *controlGroups[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};

// Controller instance
static Controller *controller = NULL;

// UI Elements
static uiWindow *window = NULL;

// Handlers
static int onClosing(uiWindow *window, void *data) {
    (void)window;
    (void)data;
    uiQuit();
    exit(0);
}

void guiUpdate() {
    for (int i = 0; i < UI_CONTROL_GROUP_SIZE; i++) {
        controlGroups[i]->update();
    }
}

static void setupUi() {
    uiInitOptions o = {0};
    const char *err;
    err = uiInit(&o);
    if (err != NULL) {
        fprintf(stderr, "Error initializing libui-ng: %s\n", err);
        uiFreeInitError(err);
        exit(1);
    }
}

static void setupWindow() {
    window = uiNewWindow("Black Ops 1 Zombies Trainer", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    uiWindowSetIcon(window, "IDI_ICON1");
    uiWindowOnClosing(window, onClosing, NULL);
    uiWindowSetMargined(window, 1);
    int screenWidth, screenHeight;
    uiScreenGetResolution(&screenWidth, &screenHeight);
    uiWindowSetPosition(window, screenWidth / 2 - (WINDOW_WIDTH/2), screenHeight / 2 - (WINDOW_HEIGHT/2));
    uiWindowSetResizeable(window, 1);
}

static uiControl* buildWindowContent() {
    // Control Groups
    UIControlGroup *playerControlGroup = uiPlayerBuildControlGroup();
    UIControlGroup *cheatsControlGroup = uiHacksBuildControlGroup();
    UIControlGroup *weaponsControlGroup = uiWeaponsBuildControlGroup();
    UIControlGroup *teleportControlGroup = uiTeleportBuildControlGroup();
    UIControlGroup *roundControlGroup = uiRoundBuildControlGroup();
    UIControlGroup *graphicsControlGroup = uiGraphicsBuildControlGroup();
    UIControlGroup *gameControlGroup = uiGameBuildControlGroup();

    // Save Control Groups for update
    controlGroups[0] = playerControlGroup;
    controlGroups[1] = cheatsControlGroup;
    controlGroups[2] = weaponsControlGroup;
    controlGroups[3] = teleportControlGroup;
    controlGroups[4] = roundControlGroup;
    controlGroups[5] = graphicsControlGroup;
    controlGroups[6] = gameControlGroup;
    
    // UI Groups
    uiControl *playerGroup = playerControlGroup->build(controller, window);
    uiControl *roundGroup = roundControlGroup->build(controller, window);
    uiControl *weaponsGroup = weaponsControlGroup->build(controller, window);
    uiControl *teleportGroup = teleportControlGroup->build(controller, window);
    uiControl *cheatGroup = cheatsControlGroup->build(controller, window);
    uiControl *graphicsGroup = graphicsControlGroup->build(controller, window);
    uiControl *gameGroup = gameControlGroup->build(controller, window);

    
    // --- VBox para Weapons + Teleport ---
    uiBox *twVBox = uiNewVerticalBox();
    uiBoxSetPadded(twVBox, 1);
    uiBoxAppend(twVBox, weaponsGroup, 1);
    uiBoxAppend(twVBox, teleportGroup, 1); 

    // --- Main Grid ---
    uiGrid *mainGrid = uiNewGrid();
    uiGridSetPadded(mainGrid, 1);

    // Fila 0
    uiGridAppend(mainGrid, playerGroup, 0, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(mainGrid, cheatGroup,  1, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(mainGrid, gameGroup,   2, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    // Fila 1
    uiGridAppend(mainGrid, graphicsGroup, 0, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(mainGrid, roundGroup,    1, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(mainGrid, uiControl(twVBox),        2, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    return uiControl(mainGrid);
}

UIControlGroup *guiControlGroupCreate(uiControl *(*build)(Controller *, uiWindow *), void (*update)()) {
    UIControlGroup *cg = (UIControlGroup*)malloc(sizeof(UIControlGroup));
    if (!cg) {
        LOG_ERROR("Couldn't allocate memory for UIControlGroup\n");
        return NULL;
    }
    cg->build = build;
    cg->update = update;
    return cg;
}

void guiInit(Controller *controllerInstance) {
    controller = controllerInstance;
    setupUi();
    setupWindow();
    uiControl *c = buildWindowContent();
    uiWindowSetChild(window,c);
}

void guiRun(void) {
    uiControlShow(uiControl(window));
    while (uiMainStep(1));
}

void guiCleanup(void) {
    uiUninit();
}
