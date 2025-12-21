/*
 * VM Notify Hook Module
 * Hooks the game's VM notification events function to intercept VM-related events
 */

#include <windows.h>
#include <string.h>
#include <stdbool.h>
#include "Hook.h"
#include "ipc/event.h"
#include "../utils/Log.h"

#define VM_NOTIFY_HOOK_NAME "VMNotifyHook"
#define VM_NOTIFY_HOOK_OFFSET_1 0x41D2E5
#define VM_NOTIFY_HOOK_OFFSET_2 0x8AB798
#define GET_EVENT_NAME_BY_ID_OFFSET 0x687530

// External function to send events
extern bool SendEvent(const Event* ev);

// Internal Black Ops function to get Event Name from Event ID
static const char* (__cdecl *getEventNameById)(int, int) = (const char* (__cdecl *)(int, int))GET_EVENT_NAME_BY_ID_OFFSET;

// Event names
static const char* eventNames[] = {  "start_of_round", "end_of_round", "end_game", "powerup_grabbed", "powerup_dropped", "zom_kill",  "fade_introblack", "fade_in_complete" };
static const int eventNamesCount = sizeof(eventNames) / sizeof(eventNames[0]);

// Function prototypes
static bool VMNotifyHookInstall(void);
static void __attribute__((naked)) VMNotifyHookTrampoline(void);
static void __cdecl VMNotifyHookFunction(int a1, int a2, int a3, int *a4);

// Hook instance
Hook vmNotifyHook = {
    .name = VM_NOTIFY_HOOK_NAME,
    .install = VMNotifyHookInstall
};

// Install the vmnotify hook
static bool VMNotifyHookInstall(void) {
    DWORD oldProtect, dummy;
    DWORD newOffset;
    
    // Hook location 1
    DWORD* pCallOffset1 = (DWORD*)VM_NOTIFY_HOOK_OFFSET_1;    
    VirtualProtect(pCallOffset1, 4, PAGE_EXECUTE_READWRITE, &oldProtect);    
    DWORD_PTR callInstructionAddr1 = (DWORD_PTR)pCallOffset1 + 4;
    newOffset = (DWORD)((DWORD_PTR)VMNotifyHookTrampoline - callInstructionAddr1);
    *pCallOffset1 = newOffset;
    VirtualProtect(pCallOffset1, 4, oldProtect, &dummy);
    
    // Hook location 2
    DWORD* pCallOffset2 = (DWORD*)VM_NOTIFY_HOOK_OFFSET_2;    
    VirtualProtect(pCallOffset2, 4, PAGE_EXECUTE_READWRITE, &oldProtect);    
    DWORD_PTR callInstructionAddr2 = (DWORD_PTR)pCallOffset2 + 4;
    newOffset = (DWORD)((DWORD_PTR)VMNotifyHookTrampoline - callInstructionAddr2);
    *pCallOffset2 = newOffset;
    VirtualProtect(pCallOffset2, 4, oldProtect, &dummy);
    
    return true;
}

// Trampoline function to call our hook function
static void __attribute__((naked)) VMNotifyHookTrampoline(void) {
    asm volatile(
            "pushl %%ebp\n\t"               // Save stack frame pointer
            "movl %%esp, %%ebp\n\t"         // Set up new stack frame
            "pushal\n\t"                    // Save all registers
            "pushl 0x10(%%ebp)\n\t"         // push pEventValue
            "pushl 0xC(%%ebp)\n\t"          // push eventId
            "pushl 0x8(%%ebp)\n\t"          // push unused
            "pushl %%eax\n\t"               // push invalidEvent
            "call %P0\n\t"                  // Call our hook function
            "addl $16, %%esp\n\t"           // Clean up stack (4 arguments * 4 bytes)
            "popal\n\t"                     // Restore all registers
            "popl %%ebp\n\t"                // Restore stack frame pointer
            "movl $0x8A87C0, %%ecx\n\t"     // Original Black Ops 1 VM Notify function address
            "jmp *%%ecx\n\t"                // Jump to original function
            : 
            : "i" (VMNotifyHookFunction)
            : "ecx"
        );
}

// Our hook function called on VM notify
static void __cdecl VMNotifyHookFunction(int invalidEvent, int unused, int eventId, int *pEventValue) {
    (void)unused; // Unused parameter (not referenced in assembly)
    if (invalidEvent || !eventId) return;

    const char *eventName = getEventNameById(eventId, 0);
    if (!eventName) return;
    LOG_INFO("%s\n", eventName);
    for (int i = 0; i < eventNamesCount; i++) {
        if (strcmp(eventName, eventNames[i]) != 0 && strncmp(eventName, "bo1zt::", 7)) continue;
        Event ev = {0};
        ev.type = EVENT_VM_NOTIFY;
        ev.timestamp = HookGetTimestamp();
        strncpy(ev.data.vmNotify.eventName, eventName, EVENT_NAME_MAX_SIZE - 1);
        ev.data.vmNotify.eventValue = pEventValue ? *pEventValue : 0;
        if (!SendEvent(&ev)) {
            LOG_ERROR("Failed to send VM notify event");
        }
        return;
    }
}
