#ifndef CHEAT_H_
#define CHEAT_H_

#include <stdint.h>
#include <stddef.h>

#define MAX_CHEAT_VALUE_SIZE 1024 // 1KB max size for flexibility
#define MAX_CHEAT_ASM_INSTRUCTION_SET_SIZE 1024 // 1KB max size for flexibility
#define ROUND_CHANGE_PATTERN_SIZE 8
#define MAX_COMMANDS_LENGTH 256
#define CHAT_COLOR_SHELLCODE_OFFSET 19  // Offset of specific color byte int shellcode

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
    CHEAT_NAME_CHAT_NAME_COLOR,
    CHEAT_NAME_MAKE_BORDERLESS,
    CHEAT_NAME_UNLIMIT_FPS,
    CHEAT_NAME_DISABLE_HUD,
    CHEAT_NAME_DISABLE_FOG,
    CHEAT_NAME_FULLBRIGHT,
    CHEAT_NAME_COLORIZED,
    CHEAT_NAME_FIX_MOVEMENT_SPEED,   // this cheat is divided into two SimpleCheat (backwards + straif velocities)
    CHEAT_NAME_SHOW_FPS,
    CHEAT_NAME_PATCH_CHAT,
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
    SIMPLE_CHEAT_NAME_SET_SPEED,
    SIMPLE_CHEAT_NAME_SET_KILLS,
    SIMPLE_CHEAT_NAME_SET_HEADSHOTS,
    SIMPLE_CHEAT_NAME_CHANGE_HOSTNAME,
    SIMPLE_CHEAT_NAME_FOV,
    SIMPLE_CHEAT_NAME_FOV_SCALE,
    SIMPLE_CHEAT_NAME_FPS_CAP,
    // Custom implementation.
    SIMPLE_CHEAT_NAME_GIVE_WEAPON,
    SIMPLE_CHEAT_NAME_GIVE_AMMO,
    SIMPLE_CHEAT_NAME_TELEPORT,
    SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_BACKGROUND,
    SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P1,
    SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P2,
    SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P3,
    SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P4,
    SIMPLE_CHEAT_NAME_CUSTOMIZER_TRANSPARENCY_POINTS,
    SIMPLE_CHEAT_NAME_CUSTOMIZER_TRANSPARENCY_SCOREBOARD,
    SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_RELOAD_PRIMARY,
    SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_RELOAD_SECONDARY,
    SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_LOW_AMMO_PRIMARY,
    SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_LOW_AMMO_SECONDARY,
    SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_NO_AMMO_PRIMARY,
    SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_NO_AMMO_SECONDARY,
    SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_FREQUENCY,
    SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_MIN,
    SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_MAX,
    SIMPLE_CHEAT_NAME_CHARACTER,
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

typedef struct {
    uintptr_t regionOffset;
    size_t regionSize;
    uint8_t pattern[ROUND_CHANGE_PATTERN_SIZE];
    size_t patternSize;
} RoundCheat;

typedef struct {
    // Level info
    uintptr_t levelName;
    uintptr_t levelElapsed;
    uintptr_t movementSpeed;
    // Game info
    uintptr_t isZombiesGameOngoingOffset;
    uintptr_t isZombiesGamePausedOffset;
    uintptr_t nResetsOffset;
    // Claymore count
    uintptr_t entityCountOffset;
    uintptr_t entityBaseOffset;
    // Snapshot entities
    uintptr_t currentSnapshotEntitiesOffset;
    uintptr_t maxSnapshotEntitiesOffset;
    // Chat status
    uintptr_t chatStatusOffset;
    // Chat input buffer - We write in this address
    uintptr_t chatInputBufferOffset;
} GameCheat;

typedef struct {
    uintptr_t baseOffset;
    uint32_t offset;
} CustomizerCheat;

typedef struct {
    CheatAsmInstructionSet instructions;
    uintptr_t offset;
} ServerCheat;

typedef enum {
    CHAT_COLOR_GRAY = 0x38,
    CHAT_COLOR_LIGHT_GRAY = 0x3E,
    CHAT_COLOR_WHITE = 0x37,
    CHAT_COLOR_BLACK = 0x30,
    CHAT_COLOR_RED = 0x31,
    CHAT_COLOR_GREEN = 0x32,
    CHAT_COLOR_YELLOW = 0x33,
    CHAT_COLOR_ORANGE = 0x3C,
    CHAT_COLOR_DARK_ORANGE = 0x40,
    CHAT_COLOR_BROWN = 0x39,
    CHAT_COLOR_CYAN = 0x35,
    CHAT_COLOR_BLUE = 0x3D,
    CHAT_COLOR_DARK_BLUE = 0x34,
    CHAT_COLOR_PINK = 0x36,
    CHAT_COLOR_PURPLE = 0x3F,
} ChatColor;

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
extern CheatAsm CHEAT_ASM_CHAT_NAME_COLOR;

// Player box
extern SimpleCheat SIMPLE_CHEAT_CHANGE_NAME;
extern SimpleCheat SIMPLE_CHEAT_SET_HEALTH;
extern SimpleCheat SIMPLE_CHEAT_SET_POINTS;
extern SimpleCheat SIMPLE_CHEAT_SET_KILLS;
extern SimpleCheat SIMPLE_CHEAT_SET_SPEED;
extern SimpleCheat SIMPLE_CHEAT_SET_HEADSHOTS;

// Teleport box
extern TeleportCheat TELEPORT_CHEAT;

// Round Change box
extern RoundCheat ROUND_CHEAT;

// Game box
extern GameCheat GAME_CHEAT;
extern Cheat CHEAT_FIX_MOVEMENT_SPEED_BACKWARDS;
extern Cheat CHEAT_FIX_MOVEMENT_SPEED_STRAIF;
extern Cheat CHEAT_SHOW_FPS;
extern SimpleCheat SIMPLE_CHEAT_CHANGE_HOSTNAME;

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

// Customizer window
extern CustomizerCheat CUSTOMIZER_CHEAT_SCORE_BACKGROUND;
extern CustomizerCheat CUSTOMIZER_CHEAT_SCORE_P1;
extern CustomizerCheat CUSTOMIZER_CHEAT_SCORE_P2;
extern CustomizerCheat CUSTOMIZER_CHEAT_SCORE_P3;
extern CustomizerCheat CUSTOMIZER_CHEAT_SCORE_P4;
extern CustomizerCheat CUSTOMIZER_CHEAT_TRANSPARENCY_POINTS;
extern CustomizerCheat CUSTOMIZER_CHEAT_TRANSPARENCY_SCOREBOARD;
extern CustomizerCheat CUSTOMIZER_CHEAT_WARN_RELOAD_PRIMARY;
extern CustomizerCheat CUSTOMIZER_CHEAT_WARN_RELOAD_SECONDARY;
extern CustomizerCheat CUSTOMIZER_CHEAT_WARN_LOW_AMMO_PRIMARY;
extern CustomizerCheat CUSTOMIZER_CHEAT_WARN_LOW_AMMO_SECONDARY;
extern CustomizerCheat CUSTOMIZER_CHEAT_WARN_NO_AMMO_PRIMARY;
extern CustomizerCheat CUSTOMIZER_CHEAT_WARN_NO_AMMO_SECONDARY;
extern CustomizerCheat CUSTOMIZER_CHEAT_WARN_FREQUENCY;
extern CustomizerCheat CUSTOMIZER_CHEAT_WARN_MIN;
extern CustomizerCheat CUSTOMIZER_CHEAT_WARN_MAX;

// Others non-ui cheats
extern Cheat CHEAT_PATCH_CHAT;
extern ServerCheat SERVER_CHEAT_SEND_COMMAND;
extern ServerCheat SERVER_CHEAT_CBUF_ADDTEXT;
extern ServerCheat SERVER_CHEAT_GET_DVAR_PTR;

int cheatGetChatColorIndex(ChatColor color);
ChatColor cheatGetChatColor(int index);
SimpleCheat cheatGetSimpleCheat(SimpleCheatName cheatName);
CustomizerCheat cheatGetCustomizerCheat(SimpleCheatName cheatName);

#endif // CHEAT_H_
