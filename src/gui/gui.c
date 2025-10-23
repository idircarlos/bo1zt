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
#include "cheats/cheats.h"
#include "graphics/graphics.h"
#include "game/game.h"

#define WINDOW_WIDTH 300
#define WINDOW_HEIGHT 250
#define UI_CONTROL_GROUP_SIZE 7

struct UIControlGroup {
    uiGroup *(*build)(Controller *, uiWindow *);
    void (*update)();
};

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
    UIControlGroup *cheatsControlGroup = uiCheatsBuildControlGroup();
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
    uiGroup *playerGroup = playerControlGroup->build(controller, window);
    uiGroup *roundGroup = roundControlGroup->build(controller, window);
    uiGroup *weaponsGroup = weaponsControlGroup->build(controller, window);
    uiGroup *teleportGroup = teleportControlGroup->build(controller, window);
    uiGroup *cheatGroup = cheatsControlGroup->build(controller, window);
    uiGroup *graphicsGroup = graphicsControlGroup->build(controller, window);
    uiGroup *gameGroup = gameControlGroup->build(controller, window);

    // Top-Left Subgrid (Player + Cheats)
    uiGrid *topLeftGrid = uiNewGrid();
    uiGridSetPadded(topLeftGrid, 1);
    uiGridAppend(topLeftGrid, uiControl(playerGroup), 0, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(topLeftGrid, uiControl(cheatGroup), 1, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    // Top-Right Subgrid (Graphics + Round Changer + Game)
    uiGrid *rightGrid = uiNewGrid();
    uiGridSetPadded(rightGrid, 1);
    uiGridAppend(rightGrid, uiControl(graphicsGroup), 0, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(rightGrid, uiControl(roundGroup), 0, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(rightGrid, uiControl(gameGroup), 1, 0, 1, 2, 1, uiAlignFill, 1, uiAlignFill);

    // Bottom-Left Subgrid (Weapons + Teleport)
    uiGrid *bottomLeftGrid = uiNewGrid();
    uiGridSetPadded(bottomLeftGrid, 1);
    uiGridAppend(bottomLeftGrid, uiControl(weaponsGroup), 0, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(bottomLeftGrid, uiControl(teleportGroup), 0, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    // --- Main grid 2x2 ---
    uiGrid *mainGrid = uiNewGrid();
    uiGridSetPadded(mainGrid, 1);
    uiGridAppend(mainGrid, uiControl(topLeftGrid), 0, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(mainGrid, uiControl(bottomLeftGrid), 0, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(mainGrid, uiControl(rightGrid), 1, 0, 1, 2, 1, uiAlignFill, 1, uiAlignFill);
    
    return uiControl(mainGrid);
}

UIControlGroup *guiControlGroupCreate(uiGroup *(*build)(Controller *, uiWindow *), void (*update)()) {
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
