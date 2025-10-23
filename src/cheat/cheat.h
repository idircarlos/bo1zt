#ifndef CHEAT_H_
#define CHEAT_H_

#include <stdint.h>
#include <stddef.h>

#define MAX_CHEAT_VALUE_SIZE 1024 // 1KB max size for flexibility
#define MAX_CHEAT_ASM_INSTRUCTION_SET_SIZE 1024 // 1KB max size for flexibility
#define NUM_WEAPON_IDS 81
#define ROUND_CHANGE_PATTERN_SIZE 8

typedef enum {
    CHEAT_NAME_GOD_MODE,
    CHEAT_NAME_INVISIBLE,
    CHEAT_NAME_NO_CLIP,
    CHEAT_NAME_NO_RECOIL,
    CHEAT_NAME_SMALL_CROSSHAIR,
    CHEAT_NAME_FAST_GAMEPLAY,
    CHEAT_NAME_NO_SHELLSHOCK,
    CHEAT_NAME_INCREASE_KNIFE_RANGE,
    CHEAT_NAME_BOX_NEVER_MOVES,
    CHEAT_NAME_THIRD_PERSON,
    CHEAT_NAME_INFINITE_AMMO,
    CHEAT_NAME_INSTANT_KILL,
    CHEAT_NAME_MAKE_BORDERLESS,
    CHEAT_NAME_UNLIMIT_FPS,
    CHEAT_NAME_DISABLE_HUD,
    CHEAT_NAME_DISABLE_FOG,
    CHEAT_NAME_FULLBRIGHT,
    CHEAT_NAME_COLORIZED,
} CheatName;

typedef union {
    uint8_t binary;
    uint8_t byte;
    uint32_t u32;
    uint64_t i64;
    float f32;
    double f64;
    char *string;
    uint8_t *array;
} CheatValue;

typedef struct {
    uint32_t offset;
    CheatValue on;
    CheatValue off;
} Cheat;

typedef struct {
    uint8_t instructions [MAX_CHEAT_ASM_INSTRUCTION_SET_SIZE]; // Max size of 1KB for flexibility
    size_t size;
} CheatAsmInstructionSet;

typedef struct {
    uint32_t offset;
    CheatAsmInstructionSet on;
    CheatAsmInstructionSet off;
} CheatAsm;

typedef enum {
    // Direct offset
    SIMPLE_CHEAT_NAME_CHANGE_NAME,
    SIMPLE_CHEAT_NAME_SET_HEALTH,
    SIMPLE_CHEAT_NAME_SET_POINTS,
    SIMPLE_CHEAT_NAME_SET_SPEED,
    SIMPLE_CHEAT_NAME_SET_KILLS,
    SIMPLE_CHEAT_NAME_SET_HEADSHOTS,
    // Offset with pointer
    SIMPLE_CHEAT_NAME_FOV,
    SIMPLE_CHEAT_NAME_FOV_SCALE,
    SIMPLE_CHEAT_NAME_FPS_CAP,
    // Custom implementation. Not part of SIMPLE_CHEAT_LOOKUP
    SIMPLE_CHEAT_NAME_GIVE_WEAPON,
    SIMPLE_CHEAT_NAME_GIVE_AMMO,
    SIMPLE_CHEAT_NAME_TELEPORT,
} SimpleCheatName;

typedef struct {
    uint32_t offset;
} SimpleCheat;

typedef struct {
    uint32_t xOffset;
    uint32_t yOffset;
    uint32_t zOffset;
} TeleportCheat;

typedef struct {
    float x;
    float y;
    float z;
} TeleportCoords;

typedef enum {
    WEAPON_UNKNOWNWEAPON = 0,
    WEAPON_DEFAULTWEAPON = 1,
    INVISIBLE_MACHINE_GUN = 2,
    AK47 = 3,
    M1911 = 4,
    MUSTAND_AND_SALLY = 8,
    MUSTAND_AND_SALLY_BROKEN = 9,
    PYTHON = 10,
    COBRA = 11,
    CZ_75 = 12,
    CALAMITY = 13,
    M14 = 14,
    MNESIA = 15,
    M16 = 16,
    SKULLCRUSHER = 17,
    G11 = 19,
    G115_GENERATOR = 20,
    FAMAS = 21,
    G16_GL35 = 22,
    AK74U = 23,
    AK74FU2 = 24,
    MP5K = 25,
    MP115_KOLLIDER = 26,
    MP40 = 27,
    THE_AFTERBURNER = 28,
    MPL = 29,
    MPL_LF = 30,
    PM63 = 31,
    TOKYO_AND_ROSE = 32,
    TOKYO_AND_ROSE_BROKEN = 33,
    SPECTRE = 34,
    PHANTOM = 35,
    CZ_75_DUAL_WIELD = 36,
    CZ_75_DUAL_WIELD_BROKEN = 37,
    CALAMITY_AND_JANE = 38,
    CALAMITY_AND_JANE_BROKEN = 39,
    STAKEOUT = 40,
    RAID = 41,
    OLYMPIA = 42,
    HADES = 43,
    SPAS_12 = 44,
    SPAZ_24 = 45,
    HS10 = 46,
    TYPHOID_AND_MARY = 47,
    TYPHOID_AND_MARY_BROKEN = 48,
    AUG = 49,
    AUG_50M3 = 50,
    GALIL = 52,
    LAMENTATION = 53,
    COMMANDO = 54,
    PREDATOR = 55,
    FN_FAL = 56,
    EPC_WN = 57,
    DRAGUNOV = 58,
    D115_DISASSEMBLER = 59,
    L96A1 = 60,
    L115_ISOLATOR = 61,
    RPK = 62,
    R115_RESONATOR = 63,
    HK21 = 64,
    H115_OSCILLATOR = 65,
    M72_LAW = 66,
    M72_ANARCHY = 67,
    CHINA_LAKE = 68,
    CHINA_BEACH = 69,
    RAY_GUN = 71,
    PORTERS_X2_RAY_GUN = 72,
    THUNDERGUN = 73,
    ZEUSCANNON = 74,
    CROSSBOW_EXPLOSIVE_TIP = 75,
    AWFUL_LAWTON = 76,
    BALLISTIC_KNIFE = 77,
    THE_KRAUSS_REFIBRILLATOR = 78,
    BALLISTIC_KNIFE_BUTCHER_KNIFE = 79,
    THE_KRAUSS_REFIBRILLATOR_BUTCHER_KNIFE = 80
} WeaponName;

typedef struct {
    uint32_t weaponOffset;
    uint32_t clipOffset;
    uint32_t ammoOffset;
} Weapon;

typedef struct {
    uint32_t currentWeaponOffset;
    Weapon weapon1;
    Weapon weapon2;
    Weapon weapon3;
} WeaponCheat;

typedef struct {
    
} GraphicsCheat;

typedef struct {
    uintptr_t regionOffset;
    size_t regionSize;
    uint8_t pattern[ROUND_CHANGE_PATTERN_SIZE];
    size_t patternSize;
} RoundCheat;

typedef struct {
    // Read-only. We only want to read from these addresses
    uintptr_t isZombiesGameActiveOffset;   // 1 if there 
    uintptr_t nResetsOffset;
} GameCheat;

// Cheats box
extern Cheat CHEAT_GOD_MODE;
extern Cheat CHEAT_INVISIBLE;
extern Cheat CHEAT_NO_CLIP;
extern Cheat CHEAT_NO_RECOIL;
extern Cheat CHEAT_FAST_GAMEPLAY;
extern Cheat CHEAT_NO_SHELLSHOCK;
extern Cheat CHEAT_INCREASE_KNIFE_RANGE;
extern Cheat CHEAT_BOX_NEVER_MOVES;
extern Cheat CHEAT_THIRD_PERSON;
extern Cheat CHEAT_INFINITE_AMMO;
extern Cheat CHEAT_SMALL_CROSSHAIR;
extern Cheat CHEAT_INSTANT_KILL;
extern CheatAsm CHEAT_ASM_INFINITE_AMMO;
extern CheatAsm CHEAT_ASM_SMALL_CROSSHAIR;
extern CheatAsm CHEAT_ASM_INSTANT_KILL;

// Player box
extern SimpleCheat SIMPLE_CHEAT_CHANGE_NAME;
extern SimpleCheat SIMPLE_CHEAT_SET_HEALTH;
extern SimpleCheat SIMPLE_CHEAT_SET_POINTS;
extern SimpleCheat SIMPLE_CHEAT_SET_KILLS;
extern SimpleCheat SIMPLE_CHEAT_SET_SPEED;
extern SimpleCheat SIMPLE_CHEAT_SET_HEADSHOTS;

// Teleport box
extern TeleportCheat TELEPORT_CHEAT;

// Weapons box
extern WeaponCheat WEAPON_CHEAT;

// Round Change box
extern RoundCheat ROUND_CHEAT;

// Game box
extern GameCheat GAME_CHEAT;

// Graphics box
extern SimpleCheat SIMPLE_CHEAT_FOV;
extern SimpleCheat SIMPLE_CHEAT_FOV_SCALE;
extern SimpleCheat SIMPLE_CHEAT_FPS_CAP;
extern Cheat CHEAT_MAKE_BORDERLESS; // Not a conventional cheat since it doesn't modify memory but creating it for consistency with rest of cheat checkboxes 
extern Cheat CHEAT_UNLIMIT_FPS;
extern Cheat CHEAT_DISABLE_HUD;
extern Cheat CHEAT_DISABLE_FOG;
extern Cheat CHEAT_FULLBRIGHT;
extern Cheat CHEAT_COLORIZED;

SimpleCheat cheatGetSimpleCheat(SimpleCheatName cheatName);
const char *cheatGetWeaponName(WeaponName weapon);
WeaponName cheatGetSanitizedWeapon(int index);

#endif // CHEAT_H_