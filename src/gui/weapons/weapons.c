#include "weapons.h"
#include "../../logger/logger.h"

// Controller instance
static Controller *controller;

// Parent Window instance
static uiWindow *parent;

// Give Weapon
static uiCombobox *weaponsCombo = NULL;
static uiButton *giveAmmoBtn = NULL;
static uiButton *giveWeaponBtn = NULL;
static uiRadioButtons *weaponSlotsRadioButtons = NULL;

// Handlers
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
    controllerGivePlayerAmmo(controller);
}

static void onWeaponSlotsSelected(uiRadioButtons *radioButtons, void *data) {
    (void)radioButtons;
    (void)data;
    uiControlEnable(uiControl(giveWeaponBtn));
}

static uiControl *build(Controller *controllerInstance, uiWindow *parentInstance) {
    controller = controllerInstance;
    parent = parentInstance;
    // --- Weapons Group ---
    uiGroup *weaponGroup = uiNewGroup("Weapons");
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

    // Botones de armas
    giveWeaponBtn = uiNewButton("Give Weapon");
    uiControlDisable(uiControl(giveWeaponBtn));
    uiButtonOnClicked(giveWeaponBtn, onGiveWeaponButtonClicked, NULL);

    giveAmmoBtn = uiNewButton("Give Ammo");
    uiButtonOnClicked(giveAmmoBtn, onGiveAmmoButtonClicked, NULL);

    weaponSlotsRadioButtons = uiNewRadioButtons();
    uiRadioButtonsAppend(weaponSlotsRadioButtons, " Slot 1 ");
    uiRadioButtonsAppend(weaponSlotsRadioButtons, " Slot 2 ");
    uiRadioButtonsAppend(weaponSlotsRadioButtons, " Slot 3 ");

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
    uiGroupSetChild(weaponGroup, uiControl(weaponsVBox));
    uiGroupSetMargined(weaponGroup, 1);
    return uiControl(weaponGroup);
}

static void update() {
    WeaponName w2 = controllerGetPlayerWeapon(controller, 2);
    WeaponName w3 = controllerGetPlayerWeapon(controller, 3);

    w2 == WEAPON_UNKNOWNWEAPON ? uiDisableRadioButton(weaponSlotsRadioButtons, 1) : uiEnableRadioButton(weaponSlotsRadioButtons, 1);
    w3 == WEAPON_UNKNOWNWEAPON ? uiDisableRadioButton(weaponSlotsRadioButtons, 2) : uiEnableRadioButton(weaponSlotsRadioButtons, 2);
}

UIControlGroup *uiWeaponsBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}
