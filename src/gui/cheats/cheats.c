#include "cheats.h"
#include "../../logger/logger.h"
#include <stdio.h>
#include <stdlib.h>

// Controller instance
static Controller *controller;

// Parent Window instance
static uiWindow *parent;

// Cheat Checkboxes
static uiCheckbox *godModeCheckbox = NULL;
static uiCheckbox *noClipCheckbox = NULL;
static uiCheckbox *invisibleCheckbox = NULL;
static uiCheckbox *infiniteAmoCheckbox = NULL;
static uiCheckbox *instantKillCheckbox = NULL;
static uiCheckbox *noRecoilCheckbox = NULL;
static uiCheckbox *smallCrosshairCheckbox = NULL;
static uiCheckbox *fastGameplayCheckbox = NULL;
static uiCheckbox *noShellshockCheckbox = NULL;
static uiCheckbox *increaseKnifeRangeCheckbox = NULL;
static uiCheckbox *boxNeverMovesCheckbox = NULL;
static uiCheckbox *thirdPersonCheckbox = NULL;

// Handlers
static void onCheckboxToggled(uiCheckbox *checkbox, void *data) {
    CheatName cheatName = (CheatName)(uintptr_t)data;
    bool enabled = uiCheckboxChecked(checkbox);
    bool success = controllerSetCheat(controller, cheatName, enabled);
    if (!success) {
        fprintf(stderr, "Failed to set cheat %d to %d\n", cheatName, enabled);
        uiCheckboxSetChecked(checkbox, !enabled); // Revert checkbox state
    }
}

static uiGroup *build(Controller *controllerInstance, uiWindow *parentInstance) {
    controller = controllerInstance;
    parent = parentInstance;
    // --- Cheats Group ---
    uiGroup *cheatGroup = uiNewGroup("Cheats");
    uiBox *cheatsBox = uiNewVerticalBox();
    uiBoxSetPadded(cheatsBox, 1);
    
    infiniteAmoCheckbox = uiNewCheckbox(" Infinite Ammo");
    instantKillCheckbox = uiNewCheckbox(" Instant Kill");
    godModeCheckbox = uiNewCheckbox(" God Mode");
    noClipCheckbox = uiNewCheckbox(" No Clip");
    invisibleCheckbox = uiNewCheckbox(" Invisible");
    noRecoilCheckbox = uiNewCheckbox(" No Recoil");
    smallCrosshairCheckbox = uiNewCheckbox(" Small Crosshair");
    fastGameplayCheckbox = uiNewCheckbox(" Fast Gameplay");
    noShellshockCheckbox = uiNewCheckbox(" No Shellshock");
    increaseKnifeRangeCheckbox = uiNewCheckbox(" Knife Range");
    boxNeverMovesCheckbox = uiNewCheckbox(" Static Box");
    thirdPersonCheckbox = uiNewCheckbox(" Third Person");

    uiCheckboxOnToggled(infiniteAmoCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_INFINITE_AMMO);
    uiCheckboxOnToggled(instantKillCheckbox, onCheckboxToggled, (void*) CHEAT_NAME_INSTANT_KILL);
    uiCheckboxOnToggled(godModeCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_GOD_MODE);
    uiCheckboxOnToggled(noClipCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_NO_CLIP);
    uiCheckboxOnToggled(invisibleCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_INVISIBLE);
    uiCheckboxOnToggled(noRecoilCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_NO_RECOIL);
    uiCheckboxOnToggled(smallCrosshairCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_SMALL_CROSSHAIR);
    uiCheckboxOnToggled(fastGameplayCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_FAST_GAMEPLAY);
    uiCheckboxOnToggled(noShellshockCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_NO_SHELLSHOCK);
    uiCheckboxOnToggled(increaseKnifeRangeCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_INCREASE_KNIFE_RANGE);
    uiCheckboxOnToggled(boxNeverMovesCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_BOX_NEVER_MOVES);
    uiCheckboxOnToggled(thirdPersonCheckbox, onCheckboxToggled, (void*)CHEAT_NAME_THIRD_PERSON);

    // Organizar los cheats en un grid 6x2
    uiGrid *cheatsGrid = uiNewGrid();
    uiGridSetPadded(cheatsGrid, 1);
    uiGridAppend(cheatsGrid, uiControl(infiniteAmoCheckbox), 0, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(cheatsGrid, uiControl(instantKillCheckbox), 1, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(cheatsGrid, uiControl(godModeCheckbox), 0, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(cheatsGrid, uiControl(noClipCheckbox), 1, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(cheatsGrid, uiControl(invisibleCheckbox), 0, 2, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(cheatsGrid, uiControl(noRecoilCheckbox), 1, 2, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(cheatsGrid, uiControl(boxNeverMovesCheckbox), 0, 3, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(cheatsGrid, uiControl(thirdPersonCheckbox), 1, 3, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(cheatsGrid, uiControl(smallCrosshairCheckbox), 0, 4, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(cheatsGrid, uiControl(fastGameplayCheckbox), 1, 4, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(cheatsGrid, uiControl(noShellshockCheckbox), 0, 5, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(cheatsGrid, uiControl(increaseKnifeRangeCheckbox), 1, 5, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    uiBoxAppend(cheatsBox, uiControl(cheatsGrid), 1);
    
    uiGroupSetChild(cheatGroup, uiControl(cheatsBox));
    uiGroupSetMargined(cheatGroup, 1);
    return cheatGroup;
}

static void update() {
    // Nothing
}

UIControlGroup *uiCheatsBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}

bool uiCheatsIsCheatChecked(CheatName cheat) {
    switch (cheat) {
        case CHEAT_NAME_GOD_MODE:
            return uiCheckboxChecked(godModeCheckbox);
        case CHEAT_NAME_INVISIBLE:
            return uiCheckboxChecked(invisibleCheckbox);
        case CHEAT_NAME_NO_CLIP:
            return uiCheckboxChecked(noClipCheckbox);
        case CHEAT_NAME_NO_RECOIL:
            return uiCheckboxChecked(noRecoilCheckbox);
        case CHEAT_NAME_SMALL_CROSSHAIR:
            return uiCheckboxChecked(smallCrosshairCheckbox);
        case CHEAT_NAME_FAST_GAMEPLAY:
            return uiCheckboxChecked(fastGameplayCheckbox);
        case CHEAT_NAME_NO_SHELLSHOCK:
            return uiCheckboxChecked(noShellshockCheckbox);
        case CHEAT_NAME_INCREASE_KNIFE_RANGE:
            return uiCheckboxChecked(increaseKnifeRangeCheckbox);
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
