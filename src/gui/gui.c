#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ui.h>
#include "gui.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 400

// Controller instance
static Controller *controller = NULL;

// UI Elements
static uiWindow *window = NULL;

// CheatName Checkboxes
static uiCheckbox *godmodeCheckbox = NULL;
static uiCheckbox *noclipCheckbox = NULL;
static uiCheckbox *invisibleCheckbox = NULL;
static uiCheckbox *infiniteAmoCheckbox = NULL;
static uiCheckbox *instantKillCheckbox = NULL;
static uiCheckbox *noRecoilCheckbox = NULL;
static uiCheckbox *boxNeverMovesCheckbox = NULL;
static uiCheckbox *thirdPersonCheckbox = NULL;

// Handlers

static int onClosing(uiWindow *window, void *data) {
    (void)window;
    (void)data;
    uiQuit();
    return 1;
}

static void onCheckboxToggled(uiCheckbox *checkbox, void *data) {
    CheatName cheat = (CheatName)(uintptr_t)data;
    bool enabled = uiCheckboxChecked(checkbox);
    bool success = controllerSetCheat(controller, cheat, enabled);
    if (!success) {
        fprintf(stderr, "Failed to set cheat %d to %d\n", cheat, enabled);
        uiCheckboxSetChecked(checkbox, !enabled); // Revert checkbox state
    }
}

// UI Api
bool guiIsCheatChecked(Controller *controller, CheatName cheat) {
    if (!controller) return false;
    switch (cheat) {
        case CHEAT_NAME_GOD_MODE:
            return uiCheckboxChecked(godmodeCheckbox);
        case CHEAT_NAME_INVISIBLE:
            return uiCheckboxChecked(invisibleCheckbox);
        case CHEAT_NAME_NO_CLIP:
            return uiCheckboxChecked(noclipCheckbox);
        case CHEAT_NAME_NO_RECOIL:
            return uiCheckboxChecked(noRecoilCheckbox);
        case CHEAT_NAME_BOX_NEVER_MOVES:
            return uiCheckboxChecked(boxNeverMovesCheckbox);
        case CHEAT_NAME_THIRD_PERSON:
            return uiCheckboxChecked(thirdPersonCheckbox);
        case CHEAT_NAME_INFINITE_AMMO:
            return uiCheckboxChecked(infiniteAmoCheckbox);
        case CHEAT_NAME_INSTANT_KILL:
            return uiCheckboxChecked(instantKillCheckbox);
        default:
            fprintf(stderr, "Unknown cheat %d\n", cheat);
            return false;
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
    uiWindowSetResizeable(window, 0);
}

static uiControl* buildGameTab() {
    // --- Round Changer Group ---
    uiGroup *roundGroup = uiNewGroup("Round Changer");
    uiBox *roundVBox = uiNewVerticalBox();
    uiBoxSetPadded(roundVBox, 1);

    uiGrid *roundGrid = uiNewGrid();
    uiGridSetPadded(roundGrid, 1);
    uiLabel *currentLabel = uiNewLabel("Current Round");
    uiSpinbox *currentSpin = uiNewSpinbox(0, 256);
    uiLabel *nextLabel = uiNewLabel("Next Round");
    uiSpinbox *nextSpin = uiNewSpinbox(0, 256);
    uiGridAppend(roundGrid, uiControl(currentLabel), 0, 0, 1, 1, 0, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(roundGrid, uiControl(currentSpin), 1, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(roundGrid, uiControl(nextLabel), 0, 1, 1, 1, 0, uiAlignFill, 0, uiAlignCenter);
    uiGridAppend(roundGrid, uiControl(nextSpin), 1, 1, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiBoxAppend(roundVBox, uiControl(roundGrid), 0);

    uiLabel *infoLabel = uiNewLabel(
        "Wait until Round 2 before using this.\nE.g: Current round 2, Next round 19, then press Change Round.\nComplete your current round and the next will be 20."
    );
    uiBoxAppend(roundVBox, uiControl(infoLabel), 0);

    uiButton *changeRoundBtn = uiNewButton("Change Round");
    uiBoxAppend(roundVBox, uiControl(changeRoundBtn), 0);

    uiGroupSetChild(roundGroup, uiControl(roundVBox));
    uiGroupSetMargined(roundGroup, 1);

    // --- Stats Group ---
    uiGroup *statsGroup = uiNewGroup("Stats");
    uiGrid *statsGrid = uiNewGrid();
    uiGridSetPadded(statsGrid, 1);

    uiButton *nameBtn = uiNewButton("Change Name");
    uiEntry *nameEntry = uiNewEntry();
    uiGridAppend(statsGrid, uiControl(nameBtn), 0, 0, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(statsGrid, uiControl(nameEntry), 1, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);

    uiButton *pointsBtn = uiNewButton("Set Points");
    uiSpinbox *pointsSpin = uiNewSpinbox(0, 9999999);
    uiGridAppend(statsGrid, uiControl(pointsBtn), 0, 1, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(statsGrid, uiControl(pointsSpin), 1, 1, 1, 1, 1, uiAlignFill, 0, uiAlignFill);

    uiButton *headshotsBtn = uiNewButton("Set Headshots");
    uiSpinbox *headshotsSpin = uiNewSpinbox(0, 999999);
    uiGridAppend(statsGrid, uiControl(headshotsBtn), 0, 2, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(statsGrid, uiControl(headshotsSpin), 1, 2, 1, 1, 1, uiAlignFill, 0, uiAlignFill);

    uiButton *killsBtn = uiNewButton("Set Kills");
    uiSpinbox *killsSpin = uiNewSpinbox(0, 999999);
    uiGridAppend(statsGrid, uiControl(killsBtn), 0, 3, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(statsGrid, uiControl(killsSpin), 1, 3, 1, 1, 1, uiAlignFill, 0, uiAlignFill);

    uiButton *revivesBtn = uiNewButton("Set Revives");
    uiSpinbox *revivesSpin = uiNewSpinbox(0, 9999);
    uiGridAppend(statsGrid, uiControl(revivesBtn), 0, 4, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(statsGrid, uiControl(revivesSpin), 1, 4, 1, 1, 1, uiAlignFill, 0, uiAlignFill);

    uiButton *downsBtn = uiNewButton("Set Downs");
    uiSpinbox *downsSpin = uiNewSpinbox(0, 9999);
    uiGridAppend(statsGrid, uiControl(downsBtn), 0, 5, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(statsGrid, uiControl(downsSpin), 1, 5, 1, 1, 1, uiAlignFill, 0, uiAlignFill);

    uiGroupSetChild(statsGroup, uiControl(statsGrid));
    uiGroupSetMargined(statsGroup, 1);

    // --- Cheats Group ---
    uiGroup *cheatsGroup = uiNewGroup("Cheats");
    uiBox *cheatsBox = uiNewVerticalBox();
    uiBoxSetPadded(cheatsBox, 1);
    
    infiniteAmoCheckbox = uiNewCheckbox(" Infinite Ammo");
    instantKillCheckbox = uiNewCheckbox(" Instant Kill");
    godmodeCheckbox = uiNewCheckbox(" God Mode");
    noclipCheckbox = uiNewCheckbox(" No Clip");
    invisibleCheckbox = uiNewCheckbox(" Invisible");
    noRecoilCheckbox = uiNewCheckbox(" No Recoil");
    boxNeverMovesCheckbox = uiNewCheckbox(" Box Never Moves");
    thirdPersonCheckbox = uiNewCheckbox(" Third Person");

    uiCheckboxOnToggled(infiniteAmoCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_INFINITE_AMMO);
    uiCheckboxOnToggled(instantKillCheckbox, onCheckboxToggled, (void*) CHEAT_NAME_INSTANT_KILL);
    uiCheckboxOnToggled(godmodeCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_GOD_MODE);
    uiCheckboxOnToggled(noclipCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_NO_CLIP);
    uiCheckboxOnToggled(invisibleCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_INVISIBLE);
    uiCheckboxOnToggled(noRecoilCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_NO_RECOIL);
    uiCheckboxOnToggled(boxNeverMovesCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_BOX_NEVER_MOVES);
    uiCheckboxOnToggled(thirdPersonCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_THIRD_PERSON);

    // Organizar los cheats en un grid 5x2
    uiGrid *cheatsGrid = uiNewGrid();
    uiGridSetPadded(cheatsGrid, 1);
    uiGridAppend(cheatsGrid, uiControl(infiniteAmoCheckbox), 0, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(cheatsGrid, uiControl(instantKillCheckbox), 1, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(cheatsGrid, uiControl(godmodeCheckbox), 0, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(cheatsGrid, uiControl(noclipCheckbox), 1, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(cheatsGrid, uiControl(invisibleCheckbox), 0, 2, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(cheatsGrid, uiControl(noRecoilCheckbox), 1, 2, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(cheatsGrid, uiControl(boxNeverMovesCheckbox), 0, 3, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(cheatsGrid, uiControl(thirdPersonCheckbox), 1, 3, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    uiBoxAppend(cheatsBox, uiControl(cheatsGrid), 1);
    
    uiGroupSetChild(cheatsGroup, uiControl(cheatsBox));
    uiGroupSetMargined(cheatsGroup, 1);

    // --- Weapons Group ---
    uiGroup *weaponsGroup = uiNewGroup("Weapons");
    uiBox *weaponsVBox = uiNewVerticalBox();
    uiBoxSetPadded(weaponsVBox, 1);
    uiCombobox *weaponsCombo = uiNewCombobox();
    uiComboboxAppend(weaponsCombo, "Ray Gun");
    uiComboboxAppend(weaponsCombo, "Thunder Gun");
    uiComboboxAppend(weaponsCombo, "Galil");

    uiComboboxSetSelected(weaponsCombo, 0);
    
    uiButton *giveWeaponBtn = uiNewButton("Give Weapon");
    uiBoxAppend(weaponsVBox, uiControl(weaponsCombo), 0);
    uiBoxAppend(weaponsVBox, uiControl(giveWeaponBtn), 0);
    uiGroupSetChild(weaponsGroup, uiControl(weaponsVBox));
    uiGroupSetMargined(weaponsGroup, 1);

    // --- Organizar en un horizontal box ---
    uiBox *mainBox = uiNewHorizontalBox();
    uiBoxSetPadded(mainBox, 1);

    // Organizar los 4 groups en un grid 2x2
    uiGrid *mainGrid = uiNewGrid();
    uiGridSetPadded(mainGrid, 1);
    uiGridAppend(mainGrid, uiControl(statsGroup), 0, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(mainGrid, uiControl(cheatsGroup), 1, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(mainGrid, uiControl(roundGroup), 0, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(mainGrid, uiControl(weaponsGroup), 1, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    return uiControl(mainGrid);
}

static uiControl* buildGraphicsTab() {
    uiGroup *graphicsGroup = uiNewGroup("Graphics");
    uiForm *graphicsForm = uiNewForm();
    uiFormSetPadded(graphicsForm, 1);
    uiSpinbox *fovSpin = uiNewSpinbox(0, 180);
    uiSpinbox *fovScaleSpin = uiNewSpinbox(0, 10);
    uiSpinbox *fpsSpin = uiNewSpinbox(0, 1000);
    uiCheckbox *patchSpeed = uiNewCheckbox("Patch Movement Speed");
    uiCheckbox *borderless = uiNewCheckbox("Borderless Window");
    uiButton *customizeUIBtn = uiNewButton("Customize Game UI");

    uiFormAppend(graphicsForm, "FOV", uiControl(fovSpin), 0);
    uiFormAppend(graphicsForm, "FOV Scale", uiControl(fovScaleSpin), 0);
    uiFormAppend(graphicsForm, "FPS", uiControl(fpsSpin), 0);
    uiFormAppend(graphicsForm, "", uiControl(patchSpeed), 0);
    uiFormAppend(graphicsForm, "", uiControl(borderless), 0);
    uiFormAppend(graphicsForm, "", uiControl(customizeUIBtn), 0);

    uiGroupSetChild(graphicsGroup, uiControl(graphicsForm));
    return uiControl(graphicsGroup);
}

void guiInit(Controller *controllerInstance) {
    controller = controllerInstance;
    setupUi();
    setupWindow();
}

void guiRun(void) {
    uiTab *tabs = uiNewTab();
    uiTabAppend(tabs, "Game", buildGameTab());
    uiTabSetMargined(tabs, 0, 1);
    uiTabAppend(tabs, "Graphics", buildGraphicsTab());
    uiTabSetMargined(tabs, 1, 1);

    uiWindowSetChild(window, uiControl(tabs));
    uiControlShow(uiControl(window));
    uiMain();
}

void guiCleanup(void) {
    uiUninit();
}