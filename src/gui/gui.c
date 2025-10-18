#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ui.h>
#include "gui.h"
#include "../logger/logger.h"

#define WINDOW_WIDTH 300
#define WINDOW_HEIGHT 250

// Controller instance
static Controller *controller = NULL;

// UI Elements
static uiWindow *window = NULL;

// SimpleCheatName Components (Button + Entry/Spinbox)
static uiButton *nameBtn = NULL;
static uiButton *healthBtn = NULL;
static uiButton *pointsBtn = NULL;
static uiButton *speedBtn = NULL;
static uiButton *headshotsBtn = NULL;
static uiButton *killsBtn = NULL;
static uiEntry *nameEntry = NULL;
static uiSpinbox *healthSpin = NULL;
static uiSpinbox *pointsSpin = NULL;
static uiSpinbox *speedSpin = NULL;
static uiSpinbox *headshotsSpin = NULL;
static uiSpinbox *killsSpin = NULL;

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

// Give WeaponName
static uiCombobox *weaponsCombo = NULL;
static uiButton *giveAmmoBtn = NULL;
static uiButton *giveWeaponBtn = NULL;
static uiRadioButtons *weaponSlotsRadioButtons = NULL;

// Teleport
static uiSpinbox *xSpin = NULL;
static uiSpinbox *ySpin = NULL;
static uiSpinbox *zSpin = NULL;
static uiButton *goBtn = NULL;

// Handlers
static int onClosing(uiWindow *window, void *data) {
    (void)window;
    (void)data;
    uiQuit();
    return 1;
}

static void onCheckboxToggled(uiCheckbox *checkbox, void *data) {
    CheatName cheatName = (CheatName)(uintptr_t)data;
    bool enabled = uiCheckboxChecked(checkbox);
    bool success = controllerSetCheat(controller, cheatName, enabled);
    if (!success) {
        fprintf(stderr, "Failed to set cheat %d to %d\n", cheatName, enabled);
        uiCheckboxSetChecked(checkbox, !enabled); // Revert checkbox state
    }
}

static void onPlayerChangeNameButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    char *name = uiEntryText(nameEntry);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CHANGE_NAME, (void*)name);
    uiFreeText(name);
}

static void onPlayerButtonClick(uiButton *button, void *data) {
    (void)button;
    SimpleCheatName simpleCheatName = (SimpleCheatName)(uintptr_t)data;
    LOG_INFO("Cheat %d\n", simpleCheatName);
    void *value = NULL;
    int spinBoxValue;

    LOG_INFO("val %x\n", (void*)(intptr_t)uiSpinboxValue(pointsSpin));

    // Maybe refactor this into an array of componentes and access by index using SimpleCheatName to avoid switch-case 
    switch(simpleCheatName) {
        case SIMPLE_CHEAT_NAME_SET_HEALTH:
            spinBoxValue = uiSpinboxValue(healthSpin);
            value = &spinBoxValue;
            break;
        case SIMPLE_CHEAT_NAME_SET_POINTS:
            spinBoxValue = uiSpinboxValue(pointsSpin);
            value = &spinBoxValue;
            break;
        case SIMPLE_CHEAT_NAME_SET_SPEED:
            spinBoxValue = uiSpinboxValue(speedSpin);
            value = &spinBoxValue;
            break;
        case SIMPLE_CHEAT_NAME_SET_KILLS:
            spinBoxValue = uiSpinboxValue(killsSpin);
            value = &spinBoxValue;
            break;
        case SIMPLE_CHEAT_NAME_SET_HEADSHOTS:
            spinBoxValue = uiSpinboxValue(headshotsSpin);
            value = &spinBoxValue;
            break;
        default:
            LOG_WARN("Cheat %d shouldn't be handled here or it doesn't exist\n", simpleCheatName);
    }
    controllerSetSimpleCheat(controller, simpleCheatName, value);
}

static void onGiveWeaponButtonClicked(uiButton *button, void *data) {
    (void)button;
    (void)data;
    int slot = uiRadioButtonsSelected(weaponSlotsRadioButtons) + 1;
    int index = uiComboboxSelected(weaponsCombo);
    WeaponName weapon = cheatGetSanitizedWeapon(index);
    controllerSetPlayerWeapon(controller, weapon, slot);
}

static void onGiveAmmoButtonClicked(uiButton *button, void *data) {
    (void)button;
    (void)data;
    LOG_INFO("XD\n");
    controllerGivePlayerAmmo(controller);
}

static void onWeaponSlotsSelected(uiRadioButtons *radioButtons, void *data) {
    (void)radioButtons;
    (void)data;
    uiControlEnable(uiControl(giveWeaponBtn));
}

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
    char *filePath = uiSaveFile(window);
    if (filePath == NULL) return;

    FILE *fp = fopen(filePath, "w");
    if (fp == NULL) {
        uiMsgBoxError(window, "Unexpected error", "Error saving the coords");
    }

    fprintf(fp, "%d %d %d", (int)coords->x, (int)coords->y, (int)coords->z);
    fclose(fp);
    free(coords);
    uiFreeText(filePath);
}

static void onTeleportLoadButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    char *filePath = uiOpenFile(window);
    if (!filePath) return;

    FILE *fp = fopen(filePath, "r");
    if (fp == NULL) {
        uiMsgBoxError(window, "Unexpected error", "Error loading the coords");
    }

    int x, y, z;
    if (fscanf(fp, "%d %d %d", &x, &y, &z) != 3) {
        uiMsgBoxError(window, "Unexpected error", "Error loading the coords");
    }

    uiSpinboxSetValue(xSpin, x);
    uiSpinboxSetValue(ySpin, y);
    uiSpinboxSetValue(zSpin, z);    
    uiFreeText(filePath);
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

static uiControl* buildWindowContent() {
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

    // --- Player Group ---
    uiGroup *playerGroup = uiNewGroup("Player");
    uiGrid *playerGrid = uiNewGrid();
    uiGridSetPadded(playerGrid, 1);

    nameBtn = uiNewButton("Change Name");
    healthBtn = uiNewButton("Set Health");
    pointsBtn = uiNewButton("Set Points");
    speedBtn = uiNewButton("Set Speed");
    killsBtn = uiNewButton("Set Kills");
    headshotsBtn = uiNewButton("Set Headshots");
    nameEntry = uiNewEntry();
    healthSpin = uiNewSpinbox(0, 999999);
    pointsSpin = uiNewSpinbox(0, 9999999);
    speedSpin = uiNewSpinbox(0, 999999);
    headshotsSpin = uiNewSpinbox(0, 999999);
    killsSpin = uiNewSpinbox(0, 999999);

    uiButtonOnClicked(nameBtn, onPlayerChangeNameButtonClick, (void*)SIMPLE_CHEAT_NAME_CHANGE_NAME);
    uiButtonOnClicked(healthBtn, onPlayerButtonClick, (void*)SIMPLE_CHEAT_NAME_SET_HEALTH);
    uiButtonOnClicked(pointsBtn, onPlayerButtonClick, (void*)SIMPLE_CHEAT_NAME_SET_POINTS);
    uiButtonOnClicked(speedBtn, onPlayerButtonClick, (void*)SIMPLE_CHEAT_NAME_SET_SPEED);
    uiButtonOnClicked(killsBtn, onPlayerButtonClick, (void*)SIMPLE_CHEAT_NAME_SET_KILLS);
    uiButtonOnClicked(headshotsBtn, onPlayerButtonClick, (void*)SIMPLE_CHEAT_NAME_SET_HEADSHOTS);

    // --- Grid Layout 5x2 ---
    
    // Fila 1
    uiGridAppend(playerGrid, uiControl(nameBtn),      0, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(nameEntry),    1, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(healthBtn),    0, 2, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(healthSpin),   1, 2, 1, 1, 1, uiAlignFill, 1, uiAlignFill);


    // Fila 2
    uiGridAppend(playerGrid, uiControl(pointsBtn),    0, 3, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(pointsSpin),   1, 3, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(killsBtn),     0, 4, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(killsSpin),    1, 4, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    // Fila 3
    uiGridAppend(playerGrid, uiControl(speedBtn),     0, 5, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(speedSpin),    1, 5, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(headshotsBtn), 0, 6, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(headshotsSpin),1, 6, 1, 1, 1, uiAlignFill, 1, uiAlignFill);


    uiGroupSetChild(playerGroup, uiControl(playerGrid));
    uiGroupSetMargined(playerGroup, 1);

    // --- Graphics Group ---
    uiGroup *graphicsGroup = uiNewGroup("Graphics");
    uiBox *graphicsBox = uiNewVerticalBox();
    uiBoxSetPadded(graphicsBox, 1);



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
    
    uiGroupSetChild(cheatsGroup, uiControl(cheatsBox));
    uiGroupSetMargined(cheatsGroup, 1);

    // --- Game Group ---
    uiGroup *gameGroup = uiNewGroup("Game");
    uiBox *gameBox = uiNewVerticalBox();
    uiBoxSetPadded(gameBox, 1);




    // --- Weapons Group ---
    uiGroup *weaponsGroup = uiNewGroup("Weapons");
    uiBox *weaponsVBox = uiNewVerticalBox();
    uiBoxSetPadded(weaponsVBox, 1);

    // Combobox con lista de armas
    weaponsCombo = uiNewCombobox();
    for (int i = 0; i < NUM_WEAPON_IDS; i++) {
        const char *weaponName = cheatGetWeaponName((WeaponName)i);
        if (weaponName != NULL) {
            uiComboboxAppend(weaponsCombo, weaponName);
        }
    } 
    uiComboboxSetSelected(weaponsCombo, 2);
    uiComboboxNumItems(weaponsCombo);

    // Botones de armas
    giveWeaponBtn = uiNewButton("Give Weapon");
    uiControlDisable(uiControl(giveWeaponBtn));
    uiButtonOnClicked(giveWeaponBtn, onGiveWeaponButtonClicked, NULL);

    giveAmmoBtn = uiNewButton("Give Ammo");
    uiButtonOnClicked(giveAmmoBtn, onGiveAmmoButtonClicked, NULL);

    weaponSlotsRadioButtons = uiNewRadioButtons();
    uiRadioButtonsAppend(weaponSlotsRadioButtons, " Slot 1   ");
    uiRadioButtonsAppend(weaponSlotsRadioButtons, " Slot 2   ");
    uiRadioButtonsAppend(weaponSlotsRadioButtons, " Slot 3   ");

    uiRadioButtonsOnSelected(weaponSlotsRadioButtons, onWeaponSlotsSelected, NULL);
    

    // Caja horizontal para los dos botones
    uiBox *weaponsButtonsHBox = uiNewHorizontalBox();
    uiBoxSetPadded(weaponsButtonsHBox, 1);
    uiBoxAppend(weaponsButtonsHBox, uiControl(weaponSlotsRadioButtons), 0);
    uiBoxAppend(weaponsButtonsHBox, uiControl(giveAmmoBtn), 1);
    uiBoxAppend(weaponsButtonsHBox, uiControl(giveWeaponBtn), 1);

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

    // --- Subgrid de arriba a la izquierda (Player + Cheats) ---
    uiGrid *topRightGrid = uiNewGrid();
    uiGridSetPadded(topRightGrid, 1);
    uiGridAppend(topRightGrid, uiControl(graphicsGroup), 0, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(topRightGrid, uiControl(gameGroup), 1, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    // --- Subgrid de arriba a la derecha (Cheats + Game) ---
    uiGrid *topLeftGrid = uiNewGrid();
    uiGridSetPadded(topLeftGrid, 1);
    uiGridAppend(topLeftGrid, uiControl(playerGroup), 0, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(topLeftGrid, uiControl(cheatsGroup), 1, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    // --- Subgrid de abajo a la izquierda (Weapons + Teleport) ---
    uiGrid *bottomLeftGrid = uiNewGrid();
    uiGridSetPadded(bottomLeftGrid, 1);
    uiGridAppend(bottomLeftGrid, uiControl(weaponsGroup), 0, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(bottomLeftGrid, uiControl(teleportGroup), 0, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    // --- Main grid 2x2 ---
    uiGrid *mainGrid = uiNewGrid();
    uiGridSetPadded(mainGrid, 1);
    uiGridAppend(mainGrid, uiControl(topLeftGrid), 0, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(mainGrid, uiControl(topRightGrid), 1, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(mainGrid, uiControl(bottomLeftGrid), 0, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(mainGrid, uiControl(roundGroup), 1, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    return uiControl(mainGrid);
}

void guiInit(Controller *controllerInstance) {
    controller = controllerInstance;
    setupUi();
    setupWindow();
}

void guiRun(void) {
    uiControl *c = buildWindowContent();
    uiWindowSetChild(window,c);
    uiControlShow(uiControl(window));
    while (uiMainStep(1));
    
}

void guiCleanup(void) {
    uiUninit();
}