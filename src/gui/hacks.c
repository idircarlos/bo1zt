#include "gui/hacks.h"
#include "client/cheats.h"
#include "logger.h"

// Shared HTTP client
static Client *client;

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

    if (clientSetCheat(client, cheatName, enabled) != CLIENT_OK) {
        uiCheckboxSetChecked(checkbox, !enabled);
    } else {
        guiSnapshotSetCheat(cheatName, enabled);
    }
}

static uiControl *build(Client *clientInstance, uiWindow *parentInstance) {
    client = clientInstance;
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

static void syncCheckbox(uiCheckbox *checkbox, CheatName cheat) {
    bool enabled;
    if (!guiSnapshotCheat(cheat, &enabled)) return;
    if (uiCheckboxChecked(checkbox) != enabled) {
        uiCheckboxSetChecked(checkbox, enabled);
    }
}

static void update() {
    syncCheckbox(godModeCheckbox, CHEAT_NAME_GOD_MODE);
    syncCheckbox(noClipCheckbox, CHEAT_NAME_NO_CLIP);
    syncCheckbox(invisibleCheckbox, CHEAT_NAME_INVISIBLE);
    syncCheckbox(infiniteAmoCheckbox, CHEAT_NAME_INFINITE_AMMO);
    syncCheckbox(instantKillCheckbox, CHEAT_NAME_INSTANT_KILL);
    syncCheckbox(noRecoilCheckbox, CHEAT_NAME_NO_RECOIL);
    syncCheckbox(smallCrosshairCheckbox, CHEAT_NAME_SMALL_CROSSHAIR);
    syncCheckbox(fastGameplayCheckbox, CHEAT_NAME_FAST_GAMEPLAY);
    syncCheckbox(noShellshockCheckbox, CHEAT_NAME_NO_SHELLSHOCK);
    syncCheckbox(increaseKnifeRangeCheckbox, CHEAT_NAME_INCREASE_KNIFE_RANGE);
    syncCheckbox(boxNeverMovesCheckbox, CHEAT_NAME_BOX_NEVER_MOVES);
    syncCheckbox(thirdPersonCheckbox, CHEAT_NAME_THIRD_PERSON);
}

UIControlGroup *uiHacksBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}
