/*
 * Map Change Hook Module
 * Hooks the game's map change function
 */

#include <windows.h>
#include <stdbool.h>
#include <string.h>
#include "Hook.h"
#include "ipc/event.h"
#include "../utils/Log.h"

#define MAP_CHANGE_HOOK_NAME "MapChangeHook"
#define MAP_CHANGE_HOOK_OFFSET 0x87C7A6

// External function to send events
extern bool SendEvent(const Event* ev);

// Function prototypes
static bool MapChangeHookInstall(void);
static void __attribute__((cdecl)) MapChangeHookTrampoline(void);
static void __cdecl MapChangeHookFunction(const char *mapName);

// Hook instance
Hook mapChangeHook = {
    .name = MAP_CHANGE_HOOK_NAME,
    .install = MapChangeHookInstall
};

// Install the map hook
static bool MapChangeHookInstall(void) {
    DWORD oldProtect, dummy;
    DWORD newOffset;
    
    // Hook location
    DWORD* pCallOffset = (DWORD*)MAP_CHANGE_HOOK_OFFSET;
    VirtualProtect(pCallOffset, 4, PAGE_EXECUTE_READWRITE, &oldProtect);        
    DWORD_PTR callInstructionAddr = (DWORD_PTR)pCallOffset + 4;
    newOffset = (DWORD)((DWORD_PTR)MapChangeHookTrampoline - callInstructionAddr); // Calculate first offset: target_function - (call_instruction_addr)
    *pCallOffset = newOffset;
    VirtualProtect(pCallOffset, 4, oldProtect, &dummy);
    
    return true;
}

// Trampoline function to call our hook function
static void __attribute__((naked)) __attribute__((cdecl)) MapChangeHookTrampoline(void) {
    asm volatile(
        "pushl %%ebp\n\t"               // Save stack frame pointer
        "movl %%esp, %%ebp\n\t"         // Set up new stack frame
        "pushal\n\t"                    // Save all registers
        "pushl 0x8(%%ebp)\n\t"          // Push argument (map)
        "call %P0\n\t"                  // Call our hook function
        "addl $4, %%esp\n\t"            // clean up stack (1 argument * 4 bytes)
        "popal\n\t"                     // Restore all registers
        "popl %%ebp\n\t"                // Restore stack frame pointer
        "pushl $0x50F030\n\t"           // Original Black Ops 1 chat function address
        "ret\n\t"                       // Return to original function
        :
        : "i" (MapChangeHookFunction)
    );
}

// Our hook function called on map change
static void __cdecl MapChangeHookFunction(const char *map) {
    Event ev = {0};
    ev.type = EVENT_MAP_CHANGE;
    ev.timestamp = HookGetTimestamp();
    strncpy(ev.data.mapChange.mapName, map, EVENT_MAP_NAME_MAX_SIZE - 1);
    
    if (!SendEvent(&ev)) {
        LOG_ERROR("Failed to send map change event");
    }
}
