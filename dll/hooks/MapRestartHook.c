/*
 * Map Restart Hook Module
 * Hooks the game's map restart function to detect when maps are restarted
 */

#include <windows.h>
#include <stdbool.h>
#include "Hook.h"
#include "../../shared/event.h"
#include "../utils/Log.h"

#define MAP_RESTART_HOOK_NAME "MapRestartHook"
#define MAP_RESTART_HOOK_OFFSET_1 0x479C39
#define MAP_RESTART_HOOK_OFFSET_2 0x479CBF

// External functions
extern bool SendEvent(const Event* ev);

// Function prototypes
static bool MapRestartHookInstall(void);
static void __attribute__((cdecl)) MapRestartHookTrampoline(void);
static void __cdecl MapRestartHookFunction(void);

// Hook instance
Hook mapRestartHook = {
    .name = MAP_RESTART_HOOK_NAME,
    .install = MapRestartHookInstall
};

// Install the map restart hooks
static bool MapRestartHookInstall(void) {
    DWORD oldProtect, dummy;
    DWORD newOffset;
    
    // Hook location 1
    DWORD* pCallOffset1 = (DWORD*)MAP_RESTART_HOOK_OFFSET_1;
    VirtualProtect(pCallOffset1, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
    DWORD_PTR callInstructionAddr1 = (DWORD_PTR)pCallOffset1 + 4;
    newOffset = (DWORD)((DWORD_PTR)MapRestartHookTrampoline - callInstructionAddr1);  // Calculate first offset: target_function - (call_instruction_addr)
    *pCallOffset1 = newOffset;    
    VirtualProtect(pCallOffset1, 4, oldProtect, &dummy);
    
    // Hook location 2
    DWORD* pCallOffset2 = (DWORD*)MAP_RESTART_HOOK_OFFSET_2;
    VirtualProtect(pCallOffset2, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
    DWORD_PTR callInstructionAddr2 = (DWORD_PTR)pCallOffset2 + 4;
    newOffset = (DWORD)((DWORD_PTR)MapRestartHookTrampoline - callInstructionAddr2); // Calculate second offset: target_function - (call_instruction_addr)
    *pCallOffset2 = newOffset;    
    VirtualProtect(pCallOffset2, 4, oldProtect, &dummy);
    
    return true;
}

// Trampoline function to call our hook function
static void __attribute__((naked)) __attribute__((cdecl)) MapRestartHookTrampoline(void) {
    asm volatile(
        "pushal\n\t"                // Save all registers
        "call %P0\n\t"              // Call our hook function
        "popal\n\t"                 // Restore all registers
        "movl $0x87D030, %%ecx\n\t" // Original Black Ops 1 Map Restart method address
        "jmp *%%ecx\n\t"            // Jump back to where the game expects to continue
        :
        : "i" (MapRestartHookFunction)
        : "ecx"
    );
}

// Our hook function called on map restart
static void __cdecl MapRestartHookFunction(void) {
    Event ev = HookBuildEvent(EVENT_MAP_RESTART, "%d", HookGetTimestamp());
    if (!SendEvent(&ev)) {
        LOG_ERROR("Failed to send map restart event");
    }
}
