#include "Hook.h"
#include "../utils/Log.h"
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include "../../shared/event.h"

#define TIMESTAMP_OFFSET 0x286D014

// Assembly opcodes for memory patching
// MOV EAX, 1 (0xB8 0x01 0x00 0x00) - Forces function to return 1 (success/true)
#define PATCH_MOV_EAX_1_LOW   0x01B8        // 0x01B8 - Lower part: MOV EAX, 1
#define PATCH_MOV_EAX_1_HIGH  0xC300        // 0xC300 - Higher part: padding + RET (0xC3)

// INT 3 (0xCC) - Breakpoint/trap instruction to disable functions
#define PATCH_INT3 0xCC                     // 0xCC - Breakpoint opcode

// JMP rel32 (0xE9) - Jump instruction for function redirection
#define PATCH_JMP  0xE9                     // 0xE9 - Jump opcode

// Jump offset values for specific redirections
#define PATCH_JMP_OFFSET_1 0xFFEE54DB
#define PATCH_JMP_OFFSET_2 0xFFFF007B
#define PATCH_JMP_OFFSET_3 0x0021E26B

static void PatchCriticalAddresses(void);

// Forward declarations for all hooks
extern Hook chatHook;
extern Hook mapRestartHook;
extern Hook mapChangeHook;

// Global array of all registered hooks
Hook* hooks[] = {
    &chatHook,
    &mapRestartHook,
    &mapChangeHook,
};

int hooksCount = sizeof(hooks) / sizeof(hooks[0]);

typedef struct {
    Hook* hook;
} HookThreadData;

static DWORD WINAPI HookInstallThread(LPVOID param) {
    HookThreadData* data = (HookThreadData*)param;
    Hook* hook = data->hook;
        
    bool success = hook->install();
    
    if (success) {
        LOG_INFO("Hook installed successfully: %s", hook->name);
    } else {
        LOG_ERROR("Failed to install hook: %s", hook->name);
    }
    
    free(data);
    return success ? 0 : 1;
}

// Install all hooks in separate threads
void HookInstallAll(void) {
    LOG_INFO("Patching critical addresses...");
    PatchCriticalAddresses();

    LOG_INFO("Installing %d hooks...", hooksCount);
    for (int i = 0; i < hooksCount; i++) {
        HookThreadData* data = (HookThreadData*)malloc(sizeof(HookThreadData));
        if (!data) {
            LOG_ERROR("Failed to allocate memory for hook thread data");
            continue;
        }
        
        data->hook = hooks[i];
        
        HANDLE hThread = CreateThread(NULL, 0, HookInstallThread, data, 0, NULL);
        if (hThread == NULL) {
            LOG_ERROR("Failed to create thread for hook: %s", hooks[i]->name);
            free(data);
        } else {
            CloseHandle(hThread);
        }
    }
}

int HookGetTimestamp(void) {
    int *timestampPtr = (int*)(TIMESTAMP_OFFSET);
    return *timestampPtr;
}

// Build an event with formatted data
Event HookBuildEvent(EventType type, const char* format, ...) {
    Event ev;
    ev.type = type;
    
    va_list args;
    va_start(args, format);
    vsnprintf(ev.data, EVENT_DATA_MAX_SIZE, format, args);
    va_end(args);
    return ev;
}

/*
 * Apply memory protection patches 
 * 
 * Patch types:
 * - MOV EAX, 1; RET: Makes functions return 1 (success) immediately, bypassing validation logic
 * - INT 3: Disables functions by inserting a breakpoint instruction
 * - JMP: Redirects function execution to different addresses
 */
static void PatchCriticalAddresses(void) {
    DWORD oldProtect, dummy;
    
    // MOV EAX, 1; RET patterns (bypass validation functions)
    VirtualProtect((LPVOID)0x662F20, 6u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x662F20 = PATCH_MOV_EAX_1_LOW;
    *(DWORD*)0x662F24 = PATCH_MOV_EAX_1_HIGH;
    VirtualProtect((LPVOID)0x662F20, 6u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x4DFD60, 6u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x4DFD60 = PATCH_MOV_EAX_1_LOW;
    *(DWORD*)0x4DFD64 = PATCH_MOV_EAX_1_HIGH;
    VirtualProtect((LPVOID)0x4DFD60, 6u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x53F880, 6u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x53F880 = PATCH_MOV_EAX_1_LOW;
    *(DWORD*)0x53F884 = PATCH_MOV_EAX_1_HIGH;
    VirtualProtect((LPVOID)0x53F880, 6u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x5A5360, 6u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x5A5360 = PATCH_MOV_EAX_1_LOW;
    *(DWORD*)0x5A5364 = PATCH_MOV_EAX_1_HIGH;
    VirtualProtect((LPVOID)0x5A5360, 6u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x64F6A0, 6u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x64F6A0 = PATCH_MOV_EAX_1_LOW;
    *(DWORD*)0x64F6A4 = PATCH_MOV_EAX_1_HIGH;
    VirtualProtect((LPVOID)0x64F6A0, 6u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x5614A0, 6u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x5614A0 = PATCH_MOV_EAX_1_LOW;
    *(DWORD*)0x5614A4 = PATCH_MOV_EAX_1_HIGH;
    VirtualProtect((LPVOID)0x5614A0, 6u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x417360, 6u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x417360 = PATCH_MOV_EAX_1_LOW;
    *(DWORD*)0x417364 = PATCH_MOV_EAX_1_HIGH;
    VirtualProtect((LPVOID)0x417360, 6u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x56AB40, 6u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x56AB40 = PATCH_MOV_EAX_1_LOW;
    *(DWORD*)0x56AB44 = PATCH_MOV_EAX_1_HIGH;
    VirtualProtect((LPVOID)0x56AB40, 6u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x59BEB0, 6u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x59BEB0 = PATCH_MOV_EAX_1_LOW;
    *(DWORD*)0x59BEB4 = PATCH_MOV_EAX_1_HIGH;
    VirtualProtect((LPVOID)0x59BEB0, 6u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x676740, 6u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x676740 = PATCH_MOV_EAX_1_LOW;
    *(DWORD*)0x676744 = PATCH_MOV_EAX_1_HIGH;
    VirtualProtect((LPVOID)0x676740, 6u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x5DB020, 6u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x5DB020 = PATCH_MOV_EAX_1_LOW;
    *(DWORD*)0x5DB024 = PATCH_MOV_EAX_1_HIGH;
    VirtualProtect((LPVOID)0x5DB020, 6u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x4F02C0, 6u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x4F02C0 = PATCH_MOV_EAX_1_LOW;
    *(DWORD*)0x4F02C4 = PATCH_MOV_EAX_1_HIGH;
    VirtualProtect((LPVOID)0x4F02C0, 6u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x572DF0, 6u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x572DF0 = PATCH_MOV_EAX_1_LOW;
    *(DWORD*)0x572DF4 = PATCH_MOV_EAX_1_HIGH;
    VirtualProtect((LPVOID)0x572DF0, 6u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x679B40, 6u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x679B40 = PATCH_MOV_EAX_1_LOW;
    *(DWORD*)0x679B44 = PATCH_MOV_EAX_1_HIGH;
    VirtualProtect((LPVOID)0x679B40, 6u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x4BFB50, 6u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x4BFB50 = PATCH_MOV_EAX_1_LOW;
    *(DWORD*)0x4BFB54 = PATCH_MOV_EAX_1_HIGH;
    VirtualProtect((LPVOID)0x4BFB50, 6u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x4D4B80, 6u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x4D4B80 = PATCH_MOV_EAX_1_LOW;
    *(DWORD*)0x4D4B84 = PATCH_MOV_EAX_1_HIGH;
    VirtualProtect((LPVOID)0x4D4B80, 6u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x501080, 6u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x501080 = PATCH_MOV_EAX_1_LOW;
    *(DWORD*)0x501084 = PATCH_MOV_EAX_1_HIGH;
    VirtualProtect((LPVOID)0x501080, 6u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x5CAB50, 6u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x5CAB50 = PATCH_MOV_EAX_1_LOW;
    *(DWORD*)0x5CAB54 = PATCH_MOV_EAX_1_HIGH;
    VirtualProtect((LPVOID)0x5CAB50, 6u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x4C0DE0, 6u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x4C0DE0 = PATCH_MOV_EAX_1_LOW;
    *(DWORD*)0x4C0DE4 = PATCH_MOV_EAX_1_HIGH;
    VirtualProtect((LPVOID)0x4C0DE0, 6u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x41CEB0, 6u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x41CEB0 = PATCH_MOV_EAX_1_LOW;
    *(DWORD*)0x41CEB4 = PATCH_MOV_EAX_1_HIGH;
    VirtualProtect((LPVOID)0x41CEB0, 6u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x41CF50, 6u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x41CF50 = PATCH_MOV_EAX_1_LOW;
    *(DWORD*)0x41CF54 = PATCH_MOV_EAX_1_HIGH;
    VirtualProtect((LPVOID)0x41CF50, 6u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x427E00, 6u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x427E00 = PATCH_MOV_EAX_1_LOW;
    *(DWORD*)0x427E04 = PATCH_MOV_EAX_1_HIGH;
    VirtualProtect((LPVOID)0x427E00, 6u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x437350, 6u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x437350 = PATCH_MOV_EAX_1_LOW;
    *(DWORD*)0x437354 = PATCH_MOV_EAX_1_HIGH;
    VirtualProtect((LPVOID)0x437350, 6u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x444E80, 6u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x444E80 = PATCH_MOV_EAX_1_LOW;
    *(DWORD*)0x444E84 = PATCH_MOV_EAX_1_HIGH;
    VirtualProtect((LPVOID)0x444E80, 6u, oldProtect, &dummy);

    // INT 3 (disable functions with breakpoint)
    VirtualProtect((LPVOID)0x46C9A0, 1u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(BYTE*)0x46C9A0 = PATCH_INT3;
    VirtualProtect((LPVOID)0x46C9A0, 1u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x5F3290, 1u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(BYTE*)0x5F3290 = PATCH_INT3;
    VirtualProtect((LPVOID)0x5F3290, 1u, oldProtect, &dummy);
    
    // JMP instructions (function redirection)
    VirtualProtect((LPVOID)0x60CC10, 1u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(BYTE*)0x60CC10 = PATCH_JMP;
    VirtualProtect((LPVOID)0x60CC10, 1u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x60CC11, 4u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x60CC11 = PATCH_JMP_OFFSET_1;
    VirtualProtect((LPVOID)0x60CC11, 4u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x63DCC0, 1u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(BYTE*)0x63DCC0 = PATCH_JMP;
    VirtualProtect((LPVOID)0x63DCC0, 1u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x63DCC1, 4u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x63DCC1 = PATCH_JMP_OFFSET_2;
    VirtualProtect((LPVOID)0x63DCC1, 4u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x580460, 1u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(BYTE*)0x580460 = PATCH_JMP;
    VirtualProtect((LPVOID)0x580460, 1u, oldProtect, &dummy);
    
    VirtualProtect((LPVOID)0x580461, 4u, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(DWORD*)0x580461 = PATCH_JMP_OFFSET_3;
    VirtualProtect((LPVOID)0x580461, 4u, oldProtect, &dummy);    
}
