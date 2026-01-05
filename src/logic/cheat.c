#include "logic/cheat.h"

// Cheats Offsets
#define CHEAT_OFFSET_GOD_MODE 0x01A79868
#define CHEAT_OFFSET_INVISIBLE CHEAT_OFFSET_GOD_MODE
#define CHEAT_OFFSET_NO_CLIP 0x01C0A74C
#define CHEAT_OFFSET_NO_RECOIL 0x006562F0
#define CHEAT_OFFSET_FAST_GAMEPLAY 0x0248173C
#define CHEAT_OFFSET_NO_SHELLSHOCK 0x02F67C5C
#define CHEAT_OFFSET_INCREASE_KNIFE_RANGE 0x00BCAFE4
#define CHEAT_OFFSET_BOX_NEVER_MOVES 0x026210F4
#define CHEAT_OFFSET_THIRD_PERSON 0x026210F0
#define CHEAT_OFFSET_SMALL_CROSSHAIR 0x00BDF31C
#define CHEAT_OFFSET_UNLIMIT_FPS 0x02481760
#define CHEAT_OFFSET_DISABLE_HUD 0x02F67BF4 
#define CHEAT_OFFSET_DISABLE_FOG 0x026214B8
#define CHEAT_OFFSET_FULLBRIGHT 0x045FBDAC
#define CHEAT_OFFSET_COLORIZED 0x045FBDA8
#define CHEAT_OFFSET_FIX_MOVEMENT_SPEED_BACKWARDS 0x00BCD0DC
#define CHEAT_OFFSET_FIX_MOVEMENT_SPEED_STRAIF 0x00BCD1BC
#define CHEAT_OFFSET_SHOW_FPS 0x02F67B68
#define CHEAT_OFFSET_PATCH_CHAT 0x0079B7D6
#define CHEAT_OFFSET_INVALID 0x00000000

// Cheats Values
#define CHEAT_VALUE_GOD_MODE_ON 2081
#define CHEAT_VALUE_GOD_MODE_OFF 2080
#define CHEAT_VALUE_INVISIBLE_ON 2085
#define CHEAT_VALUE_NO_CLIP_ON 1
#define CHEAT_VALUE_NO_CLIP_OFF 0
#define CHEAT_VALUE_NO_RECOIL_ON 117
#define CHEAT_VALUE_NO_RECOIL_OFF 116
#define CHEAT_VALUE_FAST_GAMEPLAY_ON 2.0f
#define CHEAT_VALUE_FAST_GAMEPLAY_OFF 1.0f
#define CHEAT_VALUE_NO_SHELLSHOCK_ON 0
#define CHEAT_VALUE_NO_SHELLSHOCK_OFF 1
#define CHEAT_VALUE_INCREASE_KNIFE_RANGE_ON 2000.0f
#define CHEAT_VALUE_INCREASE_KNIFE_RANGE_OFF 64.0f
#define CHEAT_VALUE_BOX_NEVER_MOVES_ON 50759716
#define CHEAT_VALUE_BOX_NEVER_MOVES_OFF 50759780
#define CHEAT_VALUE_THIRD_PERSON_ON 1
#define CHEAT_VALUE_THIRD_PERSON_OFF 0
#define CHEAT_VALUE_SMALL_CROSSHAIR_ON 0.009999999776f
#define CHEAT_VALUE_SMALL_CROSSHAIR_OFF 0.6499999762f
#define CHEAT_VALUE_UNLIMIT_FPS_ON 0
#define CHEAT_VALUE_UNLIMIT_FPS_OFF CHEAT_VALUE_INVALID // There is no OFF value fot this, instead we should take the value from the FPS Cap entry
#define CHEAT_VALUE_DISABLE_HUD_ON 0
#define CHEAT_VALUE_DISABLE_HUD_OFF 1 
#define CHEAT_VALUE_DISABLE_FOG_ON 0
#define CHEAT_VALUE_DISABLE_FOG_OFF 1
#define CHEAT_VALUE_FULLBRIGHT_ON 4
#define CHEAT_VALUE_FULLBRIGHT_OFF 4278846730
#define CHEAT_VALUE_COLORIZED_ON 2
#define CHEAT_VALUE_COLORIZED_OFF 3
#define CHEAT_VALUE_FIX_MOVEMENT_SPEED_BACKWARDS_ON 1.0f
#define CHEAT_VALUE_FIX_MOVEMENT_SPEED_BACKWARDS_OFF 0.7f
#define CHEAT_VALUE_FIX_MOVEMENT_SPEED_STRAIF_ON 1.0f
#define CHEAT_VALUE_FIX_MOVEMENT_SPEED_STRAIF_OFF 0.8f
#define CHEAT_VALUE_SHOW_FPS_ON 1
#define CHEAT_VALUE_SHOW_FPS_OFF 0
#define CHEAT_VALUE_PATCH_CHAT_ON 0
#define CHEAT_VALUE_PATCH_CHAT_OFF 17
#define CHEAT_VALUE_INVALID 0

// Cheats Asm Instructions Addresses
#define CHEAT_ASM_OFFSET_INFINITE_AMMO 0x00697A10 // Address where the Infinite Ammo assembly instruction is located.
#define CHEAT_ASM_OFFSET_SMALL_CROSSHAIR 0x00406DEA
#define CHEAT_ASM_OFFSET_INSTANT_KILL 0x007CE731
#define CHEAT_ASM_OFFSET_INVALID 0x00000000

// Cheats Asm Instruction Sets
#define CHEAT_ASM_INFINITE_AMMO_INSTRUCTION_SET_ON { 0x90, 0x90, 0x90 } // nop; nop; nop
#define CHEAT_ASM_INFINITE_AMMO_INSTRUCTION_SET_OFF { 0x89, 0x50, 0x04 } // mov [eax+04], edx
#define CHEAT_ASM_INFINITE_AMMO_INSTRUCTION_SET_ON_SIZE 3
#define CHEAT_ASM_INFINITE_AMMO_INSTRUCTION_SET_OFF_SIZE 3
#define CHEAT_ASM_SMALL_CROSSHAIR_INSTRUCTION_SET_ON { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 } // nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;
#define CHEAT_ASM_SMALL_CROSSHAIR_INSTRUCTION_SET_OFF { 0xF7, 0x83, 0xFC, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C } // test dword ptr [ebx+4FC], 0C000000h
#define CHEAT_ASM_SMALL_CROSSHAIR_INSTRUCTION_SET_ON_SIZE 10
#define CHEAT_ASM_SMALL_CROSSHAIR_INSTRUCTION_SET_OFF_SIZE 10
#define CHEAT_ASM_INSTANT_KILL_INSTRUCTION_SET_ON { 0xC7, 0x80, 0x84, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } // mov [eax+184], 0
#define CHEAT_ASM_INSTANT_KILL_INSTRUCTION_SET_OFF { 0x29, 0x98, 0x84, 0x01, 0x00, 0x00 } // sub [eax+184], ebx
#define CHEAT_ASM_INSTANT_KILL_INSTRUCTION_SET_ON_SIZE 10
#define CHEAT_ASM_INSTANT_KILL_INSTRUCTION_SET_OFF_SIZE 6

// Simple Cheats Offsets
#define SIMPLE_CHEAT_OFFSET_CHANGE_NAME 0x01C0A678
#define SIMPLE_CHEAT_OFFSET_SET_HEALTH 0x01A7987C
#define SIMPLE_CHEAT_OFFSET_SET_POINTS 0x01C0A6C8
#define SIMPLE_CHEAT_OFFSET_SET_KILLS 0x01C0A6CC
#define SIMPLE_CHEAT_OFFSET_SET_SPEED 0x1C01810
#define SIMPLE_CHEAT_OFFSET_SET_HEADSHOTS 0x01C0A6EC
#define SIMPLE_CHEAT_OFFSET_CHANGE_HOSTNAME 0x29D379B0
#define SIMPLE_CHEAT_OFFSET_FOV 0x02FF6888
#define SIMPLE_CHEAT_OFFSET_FOV_SCALE 0x02FF66A8
#define SIMPLE_CHEAT_OFFSET_FPS_CAP 0x02481760
#define SIMPLE_CHEAT_OFFSET_CUSTOMIZER_SCORE_BASE 0x02FF82E0
#define SIMPLE_CHEAT_OFFSET_CUSTOMIZER_WARN_BASE 0x00BE3B84
#define SIMPLE_CHEAT_OFFSET_TELEPORT_X 0x01C08B6C
#define SIMPLE_CHEAT_OFFSET_TELEPORT_Y 0x01C08B68
#define SIMPLE_CHEAT_OFFSET_TELEPORT_Z 0x01C08B64

// Other Cheats
#define ROUND_CHANGE_OFFSET 0x2AB00000
#define ROUND_CHANGE_REGION_SIZE 0x000F0000
#define ROUND_CHANGE_PATTERN {  0x00, 0x00, 0x00, 0x00,   /* Current round uint32_t to be replaced */  \
                                0x3F, 0x3F, 0x12, 0x00  } /* Wildcard 0x0012???? in Little Endian */   \

// Server Cheats Asm Instruction Sets. Used for remote thread execution. 
// These are injected into allocated memory pages in the game process.
#define SERVER_CHEAT_SEND_COMMAND_INSTRUCTION_SET { 0x8B, 0xC4,                     /*    mov     eax, esp          */   \
                                                    0x83, 0xC0, 0x04,               /*    add     eax, 4            */   \
                                                    0x8B, 0x00,                     /*    mov     eax, [eax]        */   \
                                                    0xFF, 0x30,                     /*    push    dword ptr [eax]   */   \
                                                    0x83, 0xC0, 0x04,               /*    add     eax, 4            */   \
                                                    0xFF, 0x30,                     /*    push    dword ptr [eax]   */   \
                                                    0x83, 0xC0, 0x04,               /*    add     eax, 4            */   \
                                                    0xFF, 0x30,                     /*    push    dword ptr [eax]   */   \
                                                    0xE8, 0xFF, 0xFF, 0xFF, 0xFF,   /*    call    <placeholder>     */   \
                                                    0x83, 0xC4, 0x0C,               /*    add     esp, 0Ch          */   \
                                                    0xC3 }                          /*    ret                       */   \

#define SERVER_CHEAT_SEND_COMMAND_INSTRUCTION_SET_SIZE 28

#define SERVER_CHEAT_CBUF_ADDTEXT_INSTRUCTION_SET { 0x8B, 0x44, 0x24, 0x04,         /*    mov     eax, [esp + 4]     */  \
                                                    0x50,                           /*    push    eax                */  \
                                                    0x6A, 0x00,                     /*    push    0                  */  \
                                                    0xE8, 0xFF, 0xFF, 0xFF, 0xFF,   /*    call    <placeholder>      */  \
                                                    0x83, 0xC4, 0x08,               /*    add     esp, 8             */  \
                                                    0xC3 }                          /*    ret                        */  \

#define SERVER_CHEAT_CBUF_ADDTEXT_INSTRUCTION_SET_SIZE 16

#define GAME_LEVEL_NAME_OFFSET 0x2899CEC
#define GAME_LEVEL_ELAPSED_OFFSET 0x0286D014
#define GAME_MOVEMENT_SPEED_OFFSET 0x01A53900
#define GAME_IS_ZOMBIES_GAME_ACTIVE_OFFSET 0x00BCB3AC
#define GAME_IS_GAME_PAUSED_OFFSET 0x03E4CBC8
#define GAME_N_RESETS_OFFSET 0x023A5598
#define GAME_ENTITY_COUNT_OFFSET 0x01C0314C
#define GAME_ENTITY_BASE_OFFSET 0x01A7981C
#define GAME_CURRENT_SNAPSHOT_ENTITIES_OFFSET 0x0286D034
#define GAME_MAX_SNAPSHOT_ENTITIES_OFFSET 0x0286D024
#define GAME_CHAT_STATUS_OFFSET 0x02910160
#define GAME_CHAT_INPUT_BUFFER_OFFSET 0x00C723B8


Cheat CHEAT_GOD_MODE = {
    CHEAT_OFFSET_GOD_MODE,
    {.u32 = CHEAT_VALUE_GOD_MODE_ON},
    {.u32 = CHEAT_VALUE_GOD_MODE_OFF},
};

Cheat CHEAT_INVISIBLE = {
    CHEAT_OFFSET_INVISIBLE,
    {.u32 = CHEAT_VALUE_INVISIBLE_ON},
    {.u32 = CHEAT_VALUE_INVALID},
};

Cheat CHEAT_NO_CLIP = {
    CHEAT_OFFSET_NO_CLIP,
    {.byte = CHEAT_VALUE_NO_CLIP_ON},
    {.byte = CHEAT_VALUE_NO_CLIP_OFF},
};

Cheat CHEAT_NO_RECOIL = {
    CHEAT_OFFSET_NO_RECOIL,
    {.byte = CHEAT_VALUE_NO_RECOIL_ON},
    {.byte = CHEAT_VALUE_NO_RECOIL_OFF},
};

Cheat CHEAT_FAST_GAMEPLAY = {
    CHEAT_OFFSET_FAST_GAMEPLAY,
    {.f32 = CHEAT_VALUE_FAST_GAMEPLAY_ON},
    {.f32 = CHEAT_VALUE_FAST_GAMEPLAY_OFF},
};

Cheat CHEAT_NO_SHELLSHOCK = {
    CHEAT_OFFSET_NO_SHELLSHOCK,
    {.byte = CHEAT_VALUE_NO_SHELLSHOCK_ON},
    {.byte = CHEAT_VALUE_NO_SHELLSHOCK_OFF},
};

Cheat CHEAT_INCREASE_KNIFE_RANGE = {
    CHEAT_OFFSET_INCREASE_KNIFE_RANGE,
    {.f32 = CHEAT_VALUE_INCREASE_KNIFE_RANGE_ON},
    {.f32 = CHEAT_VALUE_INCREASE_KNIFE_RANGE_OFF},
};

Cheat CHEAT_BOX_NEVER_MOVES = {
    CHEAT_OFFSET_BOX_NEVER_MOVES,
    {.u32 = CHEAT_VALUE_BOX_NEVER_MOVES_ON},
    {.u32 = CHEAT_VALUE_BOX_NEVER_MOVES_OFF},
};

Cheat CHEAT_THIRD_PERSON = {
    CHEAT_OFFSET_THIRD_PERSON,
    {.byte = CHEAT_VALUE_THIRD_PERSON_ON},
    {.byte = CHEAT_VALUE_THIRD_PERSON_OFF},
};

Cheat CHEAT_SMALL_CROSSHAIR = {
    CHEAT_OFFSET_SMALL_CROSSHAIR,
    {.f32 = CHEAT_VALUE_SMALL_CROSSHAIR_ON},
    {.f32 = CHEAT_VALUE_SMALL_CROSSHAIR_OFF},
};

CheatAsm CHEAT_ASM_INFINITE_AMMO = {
    CHEAT_ASM_OFFSET_INFINITE_AMMO,
    {CHEAT_ASM_INFINITE_AMMO_INSTRUCTION_SET_ON, CHEAT_ASM_INFINITE_AMMO_INSTRUCTION_SET_ON_SIZE}, 
    {CHEAT_ASM_INFINITE_AMMO_INSTRUCTION_SET_OFF, CHEAT_ASM_INFINITE_AMMO_INSTRUCTION_SET_OFF_SIZE},
};

CheatAsm CHEAT_ASM_SMALL_CROSSHAIR = {
    CHEAT_ASM_OFFSET_SMALL_CROSSHAIR,
    {CHEAT_ASM_SMALL_CROSSHAIR_INSTRUCTION_SET_ON, CHEAT_ASM_SMALL_CROSSHAIR_INSTRUCTION_SET_ON_SIZE}, 
    {CHEAT_ASM_SMALL_CROSSHAIR_INSTRUCTION_SET_OFF, CHEAT_ASM_SMALL_CROSSHAIR_INSTRUCTION_SET_OFF_SIZE},
};

CheatAsm CHEAT_ASM_INSTANT_KILL = {
    CHEAT_ASM_OFFSET_INSTANT_KILL,
    {CHEAT_ASM_INSTANT_KILL_INSTRUCTION_SET_ON, CHEAT_ASM_INSTANT_KILL_INSTRUCTION_SET_ON_SIZE}, 
    {CHEAT_ASM_INSTANT_KILL_INSTRUCTION_SET_OFF, CHEAT_ASM_INSTANT_KILL_INSTRUCTION_SET_OFF_SIZE},
};

SimpleCheat SIMPLE_CHEAT_CHANGE_NAME = {
    .offset = SIMPLE_CHEAT_OFFSET_CHANGE_NAME,
};

SimpleCheat SIMPLE_CHEAT_SET_HEALTH = {
    .offset = SIMPLE_CHEAT_OFFSET_SET_HEALTH,
};

SimpleCheat SIMPLE_CHEAT_SET_POINTS = {
    .offset = SIMPLE_CHEAT_OFFSET_SET_POINTS,
};

SimpleCheat SIMPLE_CHEAT_SET_SPEED = {
    .offset = SIMPLE_CHEAT_OFFSET_SET_SPEED,
};

SimpleCheat SIMPLE_CHEAT_SET_KILLS = {
    .offset = SIMPLE_CHEAT_OFFSET_SET_KILLS,
};

SimpleCheat SIMPLE_CHEAT_SET_HEADSHOTS = {
    .offset = SIMPLE_CHEAT_OFFSET_SET_HEADSHOTS,
};

TeleportCheat TELEPORT_CHEAT = {
    .xOffset = SIMPLE_CHEAT_OFFSET_TELEPORT_X,
    .yOffset = SIMPLE_CHEAT_OFFSET_TELEPORT_Y,
    .zOffset = SIMPLE_CHEAT_OFFSET_TELEPORT_Z,
};

RoundCheat ROUND_CHEAT = {
    .regionOffset = ROUND_CHANGE_OFFSET,
    .regionSize = ROUND_CHANGE_REGION_SIZE,
    .pattern = ROUND_CHANGE_PATTERN,
    .patternSize = ROUND_CHANGE_PATTERN_SIZE,
};

GameCheat GAME_CHEAT = {
    .levelName = GAME_LEVEL_NAME_OFFSET,
    .levelElapsed = GAME_LEVEL_ELAPSED_OFFSET,
    .movementSpeed = GAME_MOVEMENT_SPEED_OFFSET,
    .isZombiesGameOngoingOffset = GAME_IS_ZOMBIES_GAME_ACTIVE_OFFSET,
    .isZombiesGamePausedOffset = GAME_IS_GAME_PAUSED_OFFSET,
    .nResetsOffset = GAME_N_RESETS_OFFSET,
    .entityCountOffset = GAME_ENTITY_COUNT_OFFSET,
    .entityBaseOffset = GAME_ENTITY_BASE_OFFSET,
    .currentSnapshotEntitiesOffset = GAME_CURRENT_SNAPSHOT_ENTITIES_OFFSET,
    .maxSnapshotEntitiesOffset = GAME_MAX_SNAPSHOT_ENTITIES_OFFSET,
    .chatStatusOffset = GAME_CHAT_STATUS_OFFSET,
    .chatInputBufferOffset = GAME_CHAT_INPUT_BUFFER_OFFSET,
};

Cheat CHEAT_FIX_MOVEMENT_SPEED_BACKWARDS = {
    CHEAT_OFFSET_FIX_MOVEMENT_SPEED_BACKWARDS,
    {.f32 = CHEAT_VALUE_FIX_MOVEMENT_SPEED_BACKWARDS_ON},
    {.f32 = CHEAT_VALUE_FIX_MOVEMENT_SPEED_BACKWARDS_OFF},
};

Cheat CHEAT_FIX_MOVEMENT_SPEED_STRAIF = {
    CHEAT_OFFSET_FIX_MOVEMENT_SPEED_STRAIF,
    {.f32 = CHEAT_VALUE_FIX_MOVEMENT_SPEED_STRAIF_ON},
    {.f32 = CHEAT_VALUE_FIX_MOVEMENT_SPEED_STRAIF_OFF},
};

Cheat CHEAT_SHOW_FPS = {
    CHEAT_OFFSET_SHOW_FPS,
    {.byte = CHEAT_VALUE_SHOW_FPS_ON},
    {.byte = CHEAT_VALUE_SHOW_FPS_OFF},
};

SimpleCheat SIMPLE_CHEAT_CHANGE_HOSTNAME = {
    .offset = SIMPLE_CHEAT_OFFSET_CHANGE_HOSTNAME,
};

SimpleCheat SIMPLE_CHEAT_FOV = {
    .offset = SIMPLE_CHEAT_OFFSET_FOV,
};

SimpleCheat SIMPLE_CHEAT_FOV_SCALE = {
    .offset = SIMPLE_CHEAT_OFFSET_FOV_SCALE,
};

SimpleCheat SIMPLE_CHEAT_FPS_CAP = {
    .offset = SIMPLE_CHEAT_OFFSET_FPS_CAP,
};

// Not a conventional cheat since it doesn't modify memory but creating it for consistency with rest of cheat checkboxes 
Cheat CHEAT_MAKE_BORDERLESS = {
    CHEAT_OFFSET_INVALID,
    {.byte = CHEAT_VALUE_INVALID},
    {.byte = CHEAT_VALUE_INVALID},
}; 

Cheat CHEAT_UNLIMIT_FPS = {
    CHEAT_OFFSET_UNLIMIT_FPS,
    {.u32 = CHEAT_VALUE_UNLIMIT_FPS_ON},
    {.u32 = CHEAT_VALUE_UNLIMIT_FPS_OFF},
};

Cheat CHEAT_DISABLE_HUD = {
    CHEAT_OFFSET_DISABLE_HUD,
    {.byte = CHEAT_VALUE_DISABLE_HUD_ON},
    {.byte = CHEAT_VALUE_DISABLE_HUD_OFF},
};

Cheat CHEAT_DISABLE_FOG = {
    CHEAT_OFFSET_DISABLE_FOG,
    {.byte = CHEAT_VALUE_DISABLE_FOG_ON},
    {.byte = CHEAT_VALUE_DISABLE_FOG_OFF},
};

Cheat CHEAT_FULLBRIGHT = {
    CHEAT_OFFSET_FULLBRIGHT,
    {.u32 = CHEAT_VALUE_FULLBRIGHT_ON},
    {.u32 = CHEAT_VALUE_FULLBRIGHT_OFF},
};

Cheat CHEAT_COLORIZED = {
    CHEAT_OFFSET_COLORIZED,
    {.u32 = CHEAT_VALUE_COLORIZED_ON},
    {.u32 = CHEAT_VALUE_COLORIZED_OFF},
};

CustomizerCheat CUSTOMIZER_CHEAT_SCORE_BACKGROUND = {
    .baseOffset = SIMPLE_CHEAT_OFFSET_CUSTOMIZER_SCORE_BASE,
    .offset = 0x18,
};

CustomizerCheat CUSTOMIZER_CHEAT_SCORE_P1 = {
    .baseOffset = SIMPLE_CHEAT_OFFSET_CUSTOMIZER_SCORE_BASE,
    .offset = 0xF8,
};

CustomizerCheat CUSTOMIZER_CHEAT_SCORE_P2 = {
    .baseOffset = SIMPLE_CHEAT_OFFSET_CUSTOMIZER_SCORE_BASE,
    .offset = 0x168,
};

CustomizerCheat CUSTOMIZER_CHEAT_SCORE_P3 = {
    .baseOffset = SIMPLE_CHEAT_OFFSET_CUSTOMIZER_SCORE_BASE,
    .offset = 0x1D8,
};

CustomizerCheat CUSTOMIZER_CHEAT_SCORE_P4 = {
    .baseOffset = SIMPLE_CHEAT_OFFSET_CUSTOMIZER_SCORE_BASE,
    .offset = 0x248,
};

CustomizerCheat CUSTOMIZER_CHEAT_TRANSPARENCY_POINTS = {
    .baseOffset = SIMPLE_CHEAT_OFFSET_CUSTOMIZER_SCORE_BASE,
    .offset = 0x88,
};

CustomizerCheat CUSTOMIZER_CHEAT_TRANSPARENCY_SCOREBOARD = {
    .baseOffset = SIMPLE_CHEAT_OFFSET_CUSTOMIZER_SCORE_BASE,
    .offset = 0x4E8,
};

CustomizerCheat CUSTOMIZER_CHEAT_WARN_RELOAD_PRIMARY = {
    .baseOffset = SIMPLE_CHEAT_OFFSET_CUSTOMIZER_WARN_BASE,
    .offset = 0x18,
};

CustomizerCheat CUSTOMIZER_CHEAT_WARN_RELOAD_SECONDARY = {
    .baseOffset = SIMPLE_CHEAT_OFFSET_CUSTOMIZER_WARN_BASE,
    .offset = 0x88,
};

CustomizerCheat CUSTOMIZER_CHEAT_WARN_LOW_AMMO_PRIMARY = {
    .baseOffset = SIMPLE_CHEAT_OFFSET_CUSTOMIZER_WARN_BASE,
    .offset = 0x248,
};

CustomizerCheat CUSTOMIZER_CHEAT_WARN_LOW_AMMO_SECONDARY = {
    .baseOffset = SIMPLE_CHEAT_OFFSET_CUSTOMIZER_WARN_BASE,
    .offset = 0x2B8,
};

CustomizerCheat CUSTOMIZER_CHEAT_WARN_NO_AMMO_PRIMARY = {
    .baseOffset = SIMPLE_CHEAT_OFFSET_CUSTOMIZER_WARN_BASE,
    .offset = 0x328,
};

CustomizerCheat CUSTOMIZER_CHEAT_WARN_NO_AMMO_SECONDARY = {
    .baseOffset = SIMPLE_CHEAT_OFFSET_CUSTOMIZER_WARN_BASE,
    .offset = 0x398,
};

CustomizerCheat CUSTOMIZER_CHEAT_WARN_FREQUENCY = {
    .baseOffset = SIMPLE_CHEAT_OFFSET_CUSTOMIZER_WARN_BASE,
    .offset = 0xF8,
};

CustomizerCheat CUSTOMIZER_CHEAT_WARN_MIN = {
    .baseOffset = SIMPLE_CHEAT_OFFSET_CUSTOMIZER_WARN_BASE,
    .offset = 0x1D8,
};

CustomizerCheat CUSTOMIZER_CHEAT_WARN_MAX = {
    .baseOffset = SIMPLE_CHEAT_OFFSET_CUSTOMIZER_WARN_BASE,
    .offset = 0x168,
};

Cheat CHEAT_PATCH_CHAT = {
    CHEAT_OFFSET_PATCH_CHAT,
    {.byte = CHEAT_VALUE_PATCH_CHAT_ON},
    {.byte = CHEAT_VALUE_PATCH_CHAT_OFF},
};

ServerCheat SERVER_CHEAT_SEND_COMMAND = {
    .instructions = { SERVER_CHEAT_SEND_COMMAND_INSTRUCTION_SET, SERVER_CHEAT_SEND_COMMAND_INSTRUCTION_SET_SIZE },
    .offset = 0x00543CF0,
};

ServerCheat SERVER_CHEAT_CBUF_ADDTEXT = {
    .instructions = { SERVER_CHEAT_CBUF_ADDTEXT_INSTRUCTION_SET, SERVER_CHEAT_CBUF_ADDTEXT_INSTRUCTION_SET_SIZE },
    .offset = 0x0049B930,
};

ServerCheat SERVER_CHEAT_GET_DVAR_PTR = {
    .instructions = { {}, 0 }, // There is no remote code injection for this cheat
    .offset = 0x005AE810,
};

SimpleCheat SIMPLE_CHEAT_LOOKUP [] = {  SIMPLE_CHEAT_CHANGE_NAME, SIMPLE_CHEAT_SET_HEALTH, SIMPLE_CHEAT_SET_POINTS,
                                        SIMPLE_CHEAT_SET_SPEED, SIMPLE_CHEAT_SET_KILLS, SIMPLE_CHEAT_SET_HEADSHOTS,
                                        SIMPLE_CHEAT_CHANGE_HOSTNAME, SIMPLE_CHEAT_FOV, SIMPLE_CHEAT_FOV_SCALE, SIMPLE_CHEAT_FPS_CAP,
                                    };


CustomizerCheat CUSTOMIZER_CHEAT_LOOKUP [] = {  CUSTOMIZER_CHEAT_SCORE_BACKGROUND,
                                                CUSTOMIZER_CHEAT_SCORE_P1,
                                                CUSTOMIZER_CHEAT_SCORE_P2,
                                                CUSTOMIZER_CHEAT_SCORE_P3,
                                                CUSTOMIZER_CHEAT_SCORE_P4,
                                                CUSTOMIZER_CHEAT_TRANSPARENCY_POINTS,
                                                CUSTOMIZER_CHEAT_TRANSPARENCY_SCOREBOARD,
                                                CUSTOMIZER_CHEAT_WARN_RELOAD_PRIMARY,
                                                CUSTOMIZER_CHEAT_WARN_RELOAD_SECONDARY,
                                                CUSTOMIZER_CHEAT_WARN_LOW_AMMO_PRIMARY,
                                                CUSTOMIZER_CHEAT_WARN_LOW_AMMO_SECONDARY,
                                                CUSTOMIZER_CHEAT_WARN_NO_AMMO_PRIMARY,
                                                CUSTOMIZER_CHEAT_WARN_NO_AMMO_SECONDARY,
                                                CUSTOMIZER_CHEAT_WARN_FREQUENCY,
                                                CUSTOMIZER_CHEAT_WARN_MIN,
                                                CUSTOMIZER_CHEAT_WARN_MAX,
                                            };

SimpleCheat cheatGetSimpleCheat(SimpleCheatName cheatName) {
    return SIMPLE_CHEAT_LOOKUP[cheatName];
}

CustomizerCheat cheatGetCustomizerCheat(SimpleCheatName cheatName) {
    return CUSTOMIZER_CHEAT_LOOKUP[cheatName - SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_BACKGROUND];
}
