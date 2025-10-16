#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ui.h>
#include "gui.h"
#include "../logger/logger.h"

#define WINDOW_WIDTH 300
#define WINDOW_HEIGHT 450

// Controller instance
static Controller *controller = NULL;

// UI Elements
static uiWindow *window = NULL;

// CheatName Checkboxes
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
    uiBoxAppend(roundVBox, uiControl(infoLabel), 1);

    uiButton *changeRoundBtn = uiNewButton("Change Round");
    uiBoxAppend(roundVBox, uiControl(changeRoundBtn), 1);

    uiGroupSetChild(roundGroup, uiControl(roundVBox));
    uiGroupSetMargined(roundGroup, 1);

    // --- Stats Group ---
    uiGroup *playerGroup = uiNewGroup("Player");
    uiGrid *playerGrid = uiNewGrid();
    uiGridSetPadded(playerGrid, 1);

    // Columna izquierda
    uiButton *nameBtn = uiNewButton("Change Name");
    uiEntry *nameEntry = uiNewEntry();

    // Columna izquierda
    uiButton *healthBtn = uiNewButton("Set Health");
    uiSpinbox *healthSpin = uiNewSpinbox(0, 999999);

    uiButton *moneyBtn = uiNewButton("Set Money");
    uiSpinbox *moneySpin = uiNewSpinbox(0, 999999);

    uiButton *speedBtn = uiNewButton("Set Speed");
    uiSpinbox *speedSpin = uiNewSpinbox(0, 999999);

    uiButton *pointsBtn = uiNewButton("Set Points");
    uiSpinbox *pointsSpin = uiNewSpinbox(0, 9999999);

    // Columna derecha
    uiButton *headshotsBtn = uiNewButton("Set Headshots");
    uiSpinbox *headshotsSpin = uiNewSpinbox(0, 999999);

    uiButton *killsBtn = uiNewButton("Set Kills");
    uiSpinbox *killsSpin = uiNewSpinbox(0, 999999);

    uiButton *revivesBtn = uiNewButton("Set Revives");
    uiSpinbox *revivesSpin = uiNewSpinbox(0, 9999);

    uiButton *downsBtn = uiNewButton("Set Downs");
    uiSpinbox *downsSpin = uiNewSpinbox(0, 9999);

    // --- Grid Layout 5x2 ---
    // Fila 0
    uiGridAppend(playerGrid, uiControl(nameBtn),      0, 0, 2, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(nameEntry),    2, 0, 2, 1, 1, uiAlignFill, 1, uiAlignFill);
    
    // Fila 1
    uiGridAppend(playerGrid, uiControl(healthBtn),    0, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(healthSpin),   1, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(headshotsBtn), 2, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(headshotsSpin),3, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    // Fila 2
    uiGridAppend(playerGrid, uiControl(moneyBtn),     0, 2, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(moneySpin),    1, 2, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(killsBtn),     2, 2, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(killsSpin),    3, 2, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    // Fila 3
    uiGridAppend(playerGrid, uiControl(speedBtn),     0, 3, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(speedSpin),    1, 3, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(revivesBtn),   2, 3, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(revivesSpin),  3, 3, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    // Fila 4
    uiGridAppend(playerGrid, uiControl(pointsBtn),    0, 4, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(pointsSpin),   1, 4, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(downsBtn),     2, 4, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(downsSpin),    3, 4, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    uiGroupSetChild(playerGroup, uiControl(playerGrid));
    uiGroupSetMargined(playerGroup, 1);

    // --- Cheats Group ---
    uiGroup *cheatsGroup = uiNewGroup("Cheats");
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
    increaseKnifeRangeCheckbox = uiNewCheckbox(" Increase Knife Range");
    boxNeverMovesCheckbox = uiNewCheckbox(" Box Never Moves");
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
    
    uiGroupSetChild(cheatsGroup, uiControl(cheatsBox));
    uiGroupSetMargined(cheatsGroup, 1);

    // --- Weapons Group ---
    uiGroup *weaponsGroup = uiNewGroup("Weapons");
    uiBox *weaponsVBox = uiNewVerticalBox();
    uiBoxSetPadded(weaponsVBox, 1);

    // Combobox con lista de armas
    uiCombobox *weaponsCombo = uiNewCombobox();
    uiComboboxAppend(weaponsCombo, "Ray Gun");
    uiComboboxAppend(weaponsCombo, "Thunder Gun");
    uiComboboxAppend(weaponsCombo, "Galil");
    uiComboboxSetSelected(weaponsCombo, 1);

    // Botones de armas
    uiButton *giveWeapon1Btn = uiNewButton("Give Weapon Slot 1");
    uiButton *giveWeapon2Btn = uiNewButton("Give Weapon Slot 2");
    uiButton *giveWeapon3Btn = uiNewButton("Give Weapon Slot 3");

    // Caja horizontal para los dos botones
    uiBox *weaponsButtonsHBox = uiNewHorizontalBox();
    uiBoxSetPadded(weaponsButtonsHBox, 1);
    uiBoxAppend(weaponsButtonsHBox, uiControl(giveWeapon1Btn), 1);
    uiBoxAppend(weaponsButtonsHBox, uiControl(giveWeapon2Btn), 1);
    uiBoxAppend(weaponsButtonsHBox, uiControl(giveWeapon3Btn), 1);

    // Añadir al VBox principal
    uiBoxAppend(weaponsVBox, uiControl(weaponsCombo), 0);
    uiBoxAppend(weaponsVBox, uiControl(weaponsButtonsHBox), 1);

    // Añadir el VBox al grupo
    uiGroupSetChild(weaponsGroup, uiControl(weaponsVBox));
    uiGroupSetMargined(weaponsGroup, 1);


    // --- Teleport Group ---
    uiGroup *teleportGroup = uiNewGroup("Teleport");
    uiBox *teleportHBox = uiNewHorizontalBox();
    uiBoxSetPadded(teleportHBox, 1);

    // --- Grid de coordenadas (3x2: labels + spinboxes)
    uiBox *coordsVBox = uiNewVerticalBox();
    uiBoxSetPadded(coordsVBox, 1);

    uiLabel *xLabel = uiNewLabel("X");
    uiLabel *yLabel = uiNewLabel("Y");
    uiLabel *zLabel = uiNewLabel("Z");

    uiSpinbox *xSpin = uiNewSpinbox(0, 999999);
    uiSpinbox *ySpin = uiNewSpinbox(0, 999999);
    uiSpinbox *zSpin = uiNewSpinbox(0, 999999);

    // Añadir etiquetas y spinboxes en el grid (3 filas, 2 columnas)
    uiBoxAppend(coordsVBox, uiControl(xSpin), 1);
    uiBoxAppend(coordsVBox, uiControl(ySpin), 1);
    uiBoxAppend(coordsVBox, uiControl(zSpin), 1);
    
    //uiBoxAppend(coordsVBox, uiControl(xSpin), 1, 0, 1, 1, 1, uiAlignFill, 1, uiAlignCenter);

    //uiBoxAppend(coordsVBox, uiControl(yLabel), 0, 1, 1, 1, 0, uiAlignFill, 1, uiAlignFill);
    //uiBoxAppend(coordsVBox, uiControl(ySpin), 1, 1, 1, 1, 1, uiAlignFill, 1, uiAlignCenter);

    //uiBoxAppend(coordsVBox, uiControl(zLabel), 0, 2, 1, 1, 0, uiAlignFill, 1, uiAlignFill);
    //uiBoxAppend(coordsVBox, uiControl(zSpin), 1, 2, 1, 1, 1, uiAlignFill, 1, uiAlignCenter);

    // --- Botón Go (a la derecha del grid)
    uiButton *goBtn = uiNewButton("Go");

    // --- VBox con botones Load y Save (a la derecha del botón Go)
    uiBox *saveLoadVBox = uiNewVerticalBox();
    uiBoxSetPadded(saveLoadVBox, 1);
    uiButton *loadBtn = uiNewButton("Load Position");
    uiButton *saveBtn = uiNewButton("Save Position");
    uiBoxAppend(saveLoadVBox, uiControl(loadBtn), 1);
    uiBoxAppend(saveLoadVBox, uiControl(saveBtn), 1);
    

    // --- Añadir todo al box principal (coordsVBox + Go + VBox derecha)
    uiBoxAppend(teleportHBox, uiControl(saveLoadVBox), 1);
    uiBoxAppend(teleportHBox, uiControl(coordsVBox), 1);
    uiBoxAppend(teleportHBox, uiControl(goBtn), 1);
    

    // --- Añadir al grupo ---
    uiGroupSetChild(teleportGroup, uiControl(teleportHBox));
    uiGroupSetMargined(teleportGroup, 1);

    // --- Subgrid de abajo a la izquierda (Weapons + Teleport) ---
    uiGrid *bottomLeftGrid = uiNewGrid();
    uiGridSetPadded(bottomLeftGrid, 1);
    uiGridAppend(bottomLeftGrid, uiControl(weaponsGroup), 0, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(bottomLeftGrid, uiControl(teleportGroup), 0, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    // --- Main grid 2x2 ---
    uiGrid *mainGrid = uiNewGrid();
    uiGridSetPadded(mainGrid, 1);
    uiGridAppend(mainGrid, uiControl(playerGroup), 0, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(mainGrid, uiControl(cheatsGroup), 1, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(mainGrid, uiControl(bottomLeftGrid), 0, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(mainGrid, uiControl(roundGroup), 1, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

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