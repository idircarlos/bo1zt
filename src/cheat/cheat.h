#ifndef CHEAT_H_
#define CHEAT_H_

#include <stdint.h>
#include <stddef.h>

#define MAX_CHEAT_VALUE_SIZE 1024 // 1KB max size for flexibility
#define MAX_CHEAT_ASM_INSTRUCTION_SET_SIZE 1024 // 1KB max size for flexibility

typedef enum {
    CHEAT_NAME_GOD_MODE,
    CHEAT_NAME_NO_CLIP,
    CHEAT_NAME_INVISIBLE,
    CHEAT_NAME_INFINITE_AMMO,
    CHEAT_NAME_INSTANT_KILL,
    CHEAT_NAME_NO_RECOIL,
    CHEAT_NAME_BOX_NEVER_MOVES,
    CHEAT_NAME_THIRD_PERSON,
    CHEAT_NAME_FREEZE_ZOMBIES,
    CHEAT_NAME_ZOMBIE_SPAWN,
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

extern Cheat CHEAT_GOD_MODE;
extern Cheat CHEAT_NO_CLIP;
extern Cheat CHEAT_INVISIBLE;
extern Cheat CHEAT_INFINITE_AMMO;
extern Cheat CHEAT_INSTANT_KILL;
extern Cheat CHEAT_NO_RECOIL;

extern CheatAsm CHEAT_ASM_INSTRUCTION_INFINITE_AMMO;
extern CheatAsm CHEAT_ASM_INSTRUCTION_INSTANT_KILL;

#endif // CHEAT_H_