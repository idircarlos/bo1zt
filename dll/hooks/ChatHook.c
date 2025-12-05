/*
 * Chat Hook Module
 * Hooks the game's chat function to intercept messages
 */

#include <windows.h>
#include <stdbool.h>
#include <string.h>
#include "Hook.h"
#include "../../shared/event.h"
#include "../utils/Log.h"

#define CHAT_HOOK_NAME "ChatHook"
#define CHAT_HOOK_OFFSET 0x4AF7FC

// External function to send events
extern bool SendEvent(const Event* ev);

// Function prototypes
static bool ChatHookInstallImpl(void);
static void __attribute__((cdecl)) ChatHookTrampoline(void);
static void __cdecl ChatHookFunction(int clientId, const char *message);

// Hook instance
Hook chatHook = {
    .name = CHAT_HOOK_NAME,
    .install = ChatHookInstallImpl
};

// Install the chat hook
static bool ChatHookInstallImpl(void) {
    DWORD oldProtect, dummy;
    DWORD newOffset;
    
    // Hook location
    DWORD* pCallOffset = (DWORD*)CHAT_HOOK_OFFSET;
    VirtualProtect(pCallOffset, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
    DWORD_PTR callInstructionAddr = (DWORD_PTR)pCallOffset + 4;
    newOffset = (DWORD)((DWORD_PTR)ChatHookTrampoline - callInstructionAddr); // Calculate first offset: target_function - (call_instruction_addr)
    *pCallOffset = newOffset;
    VirtualProtect(pCallOffset, 4, oldProtect, &dummy);
    
    return true;
}

// Trampoline function to call our hook function
static void __attribute__((naked)) __attribute__((cdecl)) ChatHookTrampoline(void) {
    asm volatile(
        "pushl %%ebp\n\t"               // Save stack frame pointer
        "movl %%esp, %%ebp\n\t"         // Set up new stack frame
        "pushal\n\t"                    // Save all registers
        "pushl 0x14(%%ebp)\n\t"         // Push second argument (message)
        "pushl 0x8(%%ebp)\n\t"          // Push first argument (clientId)
        "call %P0\n\t"                  // Call our hook function
        "addl $8, %%esp\n\t"            // Clean up stack (2 arguments * 4 bytes)
        "popal\n\t"                     // Restore all registers
        "popl %%ebp\n\t"                // Restore stack frame pointer
        "pushl $0x49A790\n\t"           // Original Black Ops 1 chat function address
        "ret\n\t"                       // Return to original function
        :
        : "i" (ChatHookFunction)
    );
}

// Our hook function called on chat message
static void __cdecl ChatHookFunction(int clientId, const char *message) {
    Event ev = {0};
    ev.type = EVENT_CHAT_MESSAGE;
    ev.timestamp = HookGetTimestamp();
    ev.data.chat.clientId = clientId;
    // Messages starts with a non-printable character. Copying from index 1.
    strncpy(ev.data.chat.message, &message[1], EVENT_MESSAGE_MAX_SIZE - 1);
    
    if (!SendEvent(&ev)) {
        LOG_ERROR("Failed to send chat event");
    }
}
