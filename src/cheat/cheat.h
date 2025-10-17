#ifndef CHEAT_H_
#define CHEAT_H_

#include <stdint.h>
#include <stddef.h>

#define MAX_CHEAT_VALUE_SIZE 1024 // 1KB max size for flexibility
#define MAX_CHEAT_ASM_INSTRUCTION_SET_SIZE 1024 // 1KB max size for flexibility

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
    SIMPLE_CHEAT_NAME_CHANGE_NAME,
    SIMPLE_CHEAT_NAME_SET_HEALTH,
    SIMPLE_CHEAT_NAME_SET_POINTS,
    SIMPLE_CHEAT_NAME_SET_KILLS,
    SIMPLE_CHEAT_NAME_SET_SPEED,
    SIMPLE_CHEAT_NAME_SET_HEADSHOTS,
    SIMPLE_CHEAT_NAME_GIVE_WEAPON,
    SIMPLE_CHEAT_NAME_TELEPORT
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
    WEAPON_DEFAULTWEAPON,
    INVISIBLE_MACHINE_GUN,
    AK47,
    M1911,
    MUSTAND_AND_SALLY,
    MUSTAND_AND_SALLY_BROKEN,
    PYTHON,
    COBRA,
    CZ_75,
    CALAMITY,
    M14,
    MNESIA,
    M16,
    SKULLCRUSHER,
    G11,
    G115_GENERATOR,
    FAMAS,
    G16_GL35,
    AK74U,
    AK74FU2,
    MP5K,
    MP115_KOLLIDER,
    MP40,
    THE_AFTERBURNER,
    MPL,
    MPL_LF,
    PM63,
    TOKYO_AND_ROSE,
    TOKYO_AND_ROSE_BROKEN,
    SPECTRE,
    PHANTOM,
    CZ_75_DUAL_WIELD,
    CZ_75_DUAL_WIELD_BROKEN,
    CALAMITY_AND_JANE,
    CALAMITY_AND_JANE_BROKEN,
    STAKEOUT,
    RAID,
    OLYMPIA,
    HADES,
    SPAS_12,
    SPAZ_24,
    HS10,
    TYPHOID_AND_MARY,
    TYPHOID_AND_MARY_BROKEN,
    AUG,
    AUG_50M3,
    GALIL,
    LAMENTATION,
    COMMANDO,
    PREDATOR,
    FN_FAL,
    EPC_WN,
    DRAGUNOV,
    D115_DISASSEMBLER,
    L96A1,
    L115_ISOLATOR,
    RPK,
    R115_RESONATOR,
    HK21,
    H115_OSCILLATOR,
    M72_LAW,
    M72_ANARCHY,
    CHINA_LAKE,
    CHINA_BEACH,
    RAY_GUN,
    PORTERS_X2_RAY_GUN,
    THUNDERGUN,
    ZEUSCANNON,
    CROSSBOW_EXPLOSIVE_TIP,
    AWFUL_LAWTON,
    BALLISTIC_KNIFE,
    THE_KRAUSS_REFIBRILLATOR,
    BALLISTIC_KNIFE_BUTCHER_KNIFE,
    THE_KRAUSS_REFIBRILLATOR_BUTCHER_KNIFE
} Weapon;

typedef struct {
    uint32_t currentWeaponOffset;
    uint32_t slot1Offset;
    uint32_t slot2Offset;
    uint32_t slot3Offset;
} WeaponCheat;

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

extern SimpleCheat SIMPLE_CHEAT_CHANGE_NAME;
extern SimpleCheat SIMPLE_CHEAT_SET_HEALTH;
extern SimpleCheat SIMPLE_CHEAT_SET_POINTS;
extern SimpleCheat SIMPLE_CHEAT_SET_KILLS;
extern SimpleCheat SIMPLE_CHEAT_SET_SPEED;
extern SimpleCheat SIMPLE_CHEAT_SET_HEADSHOTS;

extern TeleportCheat TELEPORT_CHEAT;
extern WeaponCheat WEAPON_CHAT;

SimpleCheat cheatGetSimpleCheat(SimpleCheatName cheatName);

#endif // CHEAT_H_