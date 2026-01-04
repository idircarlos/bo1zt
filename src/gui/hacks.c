#include "gui/hacks.h"
#include "logger.h"
#include "logic/config.h"
#include "logic/cheat/manager.h"
#include "logic/cheat/manager/actions.h"

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
    
    CheatManager *cheatManager = controllerGetCheatManager(controller);
    CheatResult result = cheatManagerSetToggle(cheatManager, cheatName, enabled);
    
    // If API failed, revert checkbox to previous state
    if (result == CHEAT_RESULT_API_FAILED) {
        uiCheckboxSetChecked(checkbox, !enabled);
    }
}

static uiControl *build(Controller *controllerInstance, uiWindow *parentInstance) {
    controller = controllerInstance;
    parent = parentInstance;
    
    uiGroup *hacksGroup = uiNewGroup("Hacks");
    uiBox *hacksBox = uiNewVerticalBox();
    uiBoxSetPadded(hacksBox, 1);
    
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

    uiGrid *hacksGrid = uiNewGrid();
    uiGridSetPadded(hacksGrid, 1);
    uiGridAppend(hacksGrid, uiControl(infiniteAmoCheckbox), 0, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(hacksGrid, uiControl(instantKillCheckbox), 1, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(hacksGrid, uiControl(godModeCheckbox), 0, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(hacksGrid, uiControl(noClipCheckbox), 1, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(hacksGrid, uiControl(invisibleCheckbox), 0, 2, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(hacksGrid, uiControl(noRecoilCheckbox), 1, 2, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(hacksGrid, uiControl(boxNeverMovesCheckbox), 0, 3, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(hacksGrid, uiControl(thirdPersonCheckbox), 1, 3, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(hacksGrid, uiControl(smallCrosshairCheckbox), 0, 4, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(hacksGrid, uiControl(fastGameplayCheckbox), 1, 4, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(hacksGrid, uiControl(noShellshockCheckbox), 0, 5, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(hacksGrid, uiControl(increaseKnifeRangeCheckbox), 1, 5, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    uiBoxAppend(hacksBox, uiControl(hacksGrid), 1);
    
    uiGroupSetChild(hacksGroup, uiControl(hacksBox));
    uiGroupSetMargined(hacksGroup, 1);
    return uiControl(hacksGroup);
}

static void update() {
    Config *config = controllerGetConfig(controller);
    HacksConfig *hacks = &config->hacks;
    
    if (uiCheckboxChecked(godModeCheckbox) != hacks->godMode) {
        uiCheckboxSetChecked(godModeCheckbox, hacks->godMode);
    }
    if (uiCheckboxChecked(noClipCheckbox) != hacks->noClip) {
        uiCheckboxSetChecked(noClipCheckbox, hacks->noClip);
    }
    if (uiCheckboxChecked(invisibleCheckbox) != hacks->invisible) {
        uiCheckboxSetChecked(invisibleCheckbox, hacks->invisible);
    }
    if (uiCheckboxChecked(infiniteAmoCheckbox) != hacks->infiniteAmmo) {
        uiCheckboxSetChecked(infiniteAmoCheckbox, hacks->infiniteAmmo);
    }
    if (uiCheckboxChecked(instantKillCheckbox) != hacks->instantKill) {
        uiCheckboxSetChecked(instantKillCheckbox, hacks->instantKill);
    }
    if (uiCheckboxChecked(noRecoilCheckbox) != hacks->noRecoil) {
        uiCheckboxSetChecked(noRecoilCheckbox, hacks->noRecoil);
    }
    if (uiCheckboxChecked(smallCrosshairCheckbox) != hacks->smallCrosshair) {
        uiCheckboxSetChecked(smallCrosshairCheckbox, hacks->smallCrosshair);
    }
    if (uiCheckboxChecked(fastGameplayCheckbox) != hacks->fastGameplay) {
        uiCheckboxSetChecked(fastGameplayCheckbox, hacks->fastGameplay);
    }
    if (uiCheckboxChecked(noShellshockCheckbox) != hacks->noShellshock) {
        uiCheckboxSetChecked(noShellshockCheckbox, hacks->noShellshock);
    }
    if (uiCheckboxChecked(increaseKnifeRangeCheckbox) != hacks->increaseKnifeRange) {
        uiCheckboxSetChecked(increaseKnifeRangeCheckbox, hacks->increaseKnifeRange);
    }
    if (uiCheckboxChecked(boxNeverMovesCheckbox) != hacks->boxNeverMoves) {
        uiCheckboxSetChecked(boxNeverMovesCheckbox, hacks->boxNeverMoves);
    }
    if (uiCheckboxChecked(thirdPersonCheckbox) != hacks->thirdPerson) {
        uiCheckboxSetChecked(thirdPersonCheckbox, hacks->thirdPerson);
    }
}

UIControlGroup *uiHacksBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}

// External API for Controller
bool uiHacksIsChecked(CheatName cheat) {
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
            LOG_ERROR("Unknown cheat %d", cheat);
            return false;
    }
}
