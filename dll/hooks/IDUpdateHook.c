/*
 * IDUpdate Hook Module
 * Hooks ID Update function to intercept game events
 */

#include <windows.h>
#include <string.h>
#include <stdbool.h>
#include "Hook.h"
#include "../utils/Log.h"
#include "ipc/event.h"

#define ID_UPDATE_HOOK_NAME "IDUpdateHook"
#define ID_UPDATE_HOOK_OFFSET 0x483E2F
#define ID_UPDATE_ROUND_NUMBER_ID 4748
#define ID_UPDATE_SOLO_LIVES_GIVEN_ID 5325
#define ID_UPDATE_CHEST_ACCESSED_ID 5100

// External function to send events
extern bool SendEvent(const Event* ev);

// Event IDs
static int eventIds[] = { ID_UPDATE_ROUND_NUMBER_ID, ID_UPDATE_SOLO_LIVES_GIVEN_ID, ID_UPDATE_CHEST_ACCESSED_ID };
static int eventIdsCount = sizeof(eventIds) / sizeof(eventIds[0]);

// Function prototypes
static bool IDUpdateHookInstall(void);
static void __attribute__((naked)) IDUpdateHookTrampoline(void);
static void IDUpdateHookFunction(int packedData, int unused, int *pEventValue);
static bool EventIsBeingMonitored(int eventId);

// Hook instance
Hook idUpdateHook = {
    .name = ID_UPDATE_HOOK_NAME,
    .install = IDUpdateHookInstall
};

// Install ID Update hook
static bool IDUpdateHookInstall(void) {
    DWORD oldProtect, dummy;
        
    // Address where we need to patch the JMP instruction
    LPVOID pPatchAddr = (LPVOID)ID_UPDATE_HOOK_OFFSET;
    VirtualProtect(pPatchAddr, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
    
    // Calculate the JMP offset to our trampoline
    DWORD targetAddr = (DWORD)IDUpdateHookTrampoline;
    DWORD patchAddrEnd = (DWORD)pPatchAddr + 5;
    DWORD offset = targetAddr - patchAddrEnd;
    
    // Build the patch: 0xE9 (JMP) followed by 4-byte offset
    BYTE patchBytes[5];
    patchBytes[0] = 0xE9;  // JMP opcode
    memcpy(&patchBytes[1], &offset, 4);
    memcpy(pPatchAddr, patchBytes, 5);
    VirtualProtect(pPatchAddr, 5, oldProtect, &dummy);
    
    return true;
}

// // Trampoline function to call our hook function
static void __attribute__((naked)) __attribute__((cdecl)) IDUpdateHookTrampoline(void) {
    asm volatile(
        "pushal\n\t"                // Save all registers
        "pushl %%esi\n\t"           // Push pEventValue
        "pushl %%eax\n\t"           // Push unused
        "pushl %%edx\n\t"           // Push eventData
        "call %P0\n\t"              // Call our hook function
        "addl $12, %%esp\n\t"       // Clean up stack (3 arguments * 4 bytes)
        "popal\n\t"                 // Restore registers
        "movl %%eax, 4(%%esi)\n\t"  // Original Black Ops 1 ID Update function address
        "popl %%esi\n\t"            // Restore ESI since was part of the removed code on Hook Installation
        "ret\n\t"                   // Ret to original function
        :
        : "i" (IDUpdateHookFunction)
    );
}

// Our hook function called on ID Update.
static void IDUpdateHookFunction(int eventData, int unused, int* pEventValue) {
    (void)unused;  // Unused parameter (not referenced in assembly)

    if (!eventData) return;

    // eventData layout: [flags (8 bits)][eventId (24 bits)]
    // This weird layout is probably caused because some compiler optimizations.
    unsigned char flags   = (eventData >> 0x18) & 0x000000FF;
    unsigned int  eventId = (eventData >> 0x08) & 0x00FFFFFF;

    // Skip if: flag bit 0 is set, eventId is zero, or event is not monitored
    if ((flags & 0x01) || (eventId == 0x000000) || !EventIsBeingMonitored(eventId)) return;

    Event ev = {0};
    ev.type = EVENT_ID_UPDATE;
    ev.timestamp = HookGetTimestamp();
    ev.data.idUpdate.eventId = eventId;
    ev.data.idUpdate.pEventValue = pEventValue;
    if (!SendEvent(&ev)) {
        LOG_ERROR("Failed to send ID update event");
    }
}

// Check if a given Event ID is being monitored
static bool EventIsBeingMonitored(int eventId) {
    for (int i = 0; i < eventIdsCount; i++) {
        if (eventIds[i] == eventId) return true;
    }
    return false;
}
