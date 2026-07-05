#include "gui/weapons.h"
#include "client/player.h"
#include "logic/game/weapon.h"

// Shared HTTP client
static Client *client;

// Parent Window instance
static uiWindow *parent;

// Give Weapon
static uiCombobox *weaponsCombo = NULL;
static uiButton *takeWeaponsBtn = NULL;
static uiButton *giveAmmoBtn = NULL;
static uiButton *giveWeaponBtn = NULL;

static const char *weaponList [] = {
    "Cymbal Monkey",
    "Black Hole",
    "Nesting Dolls",
    "Quantum Bomb",
    "M1911",
    "Mustang and Sally",
    "Python",
    "Cobra",
    "CZ75",
    "Calamity",
    "M14",
    "Mnesia",
    "M16",
    "Skullcrusher",
    "G11",
    "G115 Generator",
    "Famas",
    "G16 GL35",
    "AK74u",
    "AK74fu2",
    "MP5K",
    "MP115 Kollider",
    "MP40",
    "The Afterburner",
    "MPL",
    "MPL-LF",
    "PM63",
    "Tokyo and Rose",
    "Spectre",
    "Phantom",
    "CZ75 Dual Wield",
    "Calamity and Jame",
    "Stakeout",
    "Raid",
    "Olympia",
    "Hades",
    "Spas-12",
    "Spaz-24",
    "HS10",
    "Typhoid and Mary",
    "AUG",
    "AUG-50M3",
    "Galil",
    "Lamentation",
    "Commando",
    "Predator",
    "FN FAL",
    "EPC WN",
    "Dragunov",
    "D115 Disassembler",
    "L96A1",
    "L115 Isolator",
    "RPK",
    "R115 Resonator",
    "HK21",
    "H115 Oscilator",
    "M72 LAW",
    "M72 Anarchy",
    "China Lake",
    "China Beach",
    "Ray Gun",
    "Porter's X2 Ray Gun",
    "Thundergun",
    "ZeusCannon",
    "Crowssbow",
    "Awful Lawton",
    "Ballistic Knife",
    "The Krauss Refibrillator",
    "Ballistic Knife + Bowie",
    "The Krauss Refibrillator + Bowie"
};

static const int weaponListCount = sizeof(weaponList) / sizeof(weaponList[0]);

// Handlers
static void onTakeWeaponsButtonClicked(uiButton *button, void *data) {
    (void)button;
    (void)data;
    clientTakeWeapons(client);
}

static void onGiveWeaponButtonClicked(uiButton *button, void *data) {
    (void)button;
    (void)data;
    Weapon weapon = (Weapon)uiComboboxSelected(weaponsCombo);
    clientGiveWeapons(client, &weapon, 1);
}

static void onGiveAmmoButtonClicked(uiButton *button, void *data) {
    (void)button;
    (void)data;
    clientGiveAmmo(client);
}

static uiControl *build(Client *clientInstance, uiWindow *parentInstance) {
    client = clientInstance;
    parent = parentInstance;
    // --- Weapons Group ---
    uiGroup *weaponGroup = uiNewGroup("Weapons");
    uiBox *weaponsVBox = uiNewVerticalBox();
    uiBoxSetPadded(weaponsVBox, 1);

    // Weapon list
    weaponsCombo = uiNewCombobox();
    for (int i = 0; i < weaponListCount; i++) {
        uiComboboxAppend(weaponsCombo, weaponList[i]);
    }
    
    uiComboboxSetSelected(weaponsCombo, 0);

    takeWeaponsBtn = uiNewButton("Take Weapons");
    uiButtonOnClicked(takeWeaponsBtn, onTakeWeaponsButtonClicked, NULL);

    giveWeaponBtn = uiNewButton("Give Weapon");
    uiButtonOnClicked(giveWeaponBtn, onGiveWeaponButtonClicked, NULL);

    giveAmmoBtn = uiNewButton("Give Ammo");
    uiButtonOnClicked(giveAmmoBtn, onGiveAmmoButtonClicked, NULL);
    

    uiBox *weaponsButtonsHBox = uiNewHorizontalBox();
    uiBoxSetPadded(weaponsButtonsHBox, 1);
    uiBoxAppend(weaponsButtonsHBox, uiControl(takeWeaponsBtn), 1);
    uiBoxAppend(weaponsButtonsHBox, uiControl(giveWeaponBtn), 1);
    uiBoxAppend(weaponsButtonsHBox, uiControl(giveAmmoBtn), 1);

    uiBoxAppend(weaponsVBox, uiControl(weaponsCombo), 0);
    uiBoxAppend(weaponsVBox, uiControl(weaponsButtonsHBox), 1);

    uiGroupSetChild(weaponGroup, uiControl(weaponsVBox));
    uiGroupSetMargined(weaponGroup, 1);
    return uiControl(weaponGroup);
}

static void update() {
    // Nothing
}

UIControlGroup *uiWeaponsBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}
