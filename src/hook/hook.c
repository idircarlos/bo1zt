#include "hook.h"
#include "../logger/logger.h"
#include <stdlib.h>
#include <windows.h>

#define MIN_PAGE_SIZE 0x1000 // 4096 Bytes

static const uint8_t JMP_INSTRUCTION = 0xE9; // JMP opcode
static const uint8_t NOP_INSTRUCTION = 0x90; // NOP opcode

struct Hook {
    Process *process;
    // Hook entry point
    uintptr_t startAddress;
    uint8_t *originalBytes;
    size_t originalBytesSize;
    uint32_t jmpToHook;
    
    // Hook code
    uintptr_t allocatedPageAddress;
    uint8_t *shellCode;
    size_t shellCodeSize;
};

Hook* hookCreate(Process *process, uintptr_t startAddress, size_t size, uint8_t *shellCode, size_t shellCodeSize) {
    // Allocate a read-write-execute memory page in the target process
    uintptr_t pageAddress;
    bool success = processAllocatePage(process, shellCodeSize > MIN_PAGE_SIZE ? shellCodeSize : MIN_PAGE_SIZE, &pageAddress);
    if (!success) {
        LOG_ERROR("Failed to allocate memory page in target process.\n");
        return NULL;
    }

    // Write the shellCode to the allocated page
    success = processWrite(process, pageAddress, shellCode, shellCodeSize);
    if (!success) {
        LOG_ERROR("Failed to write shellCode to allocated memory page.\n");
        return NULL;
    }

    // Calculate the jump address back after the hook
	uint32_t jmpBackAddress = (startAddress + size) - (pageAddress + shellCodeSize + 5);

    // Place the jmp instruction + address into the allocated page, which will jmp back to the hooked function
    success = processWrite(process, pageAddress + shellCodeSize, &JMP_INSTRUCTION, sizeof(JMP_INSTRUCTION)); // JMP opcode
    if (!success) {
        LOG_ERROR("Failed to write JMP opcode to allocated memory page.\n");
        return NULL;
    }

    success = processWrite(process, pageAddress + shellCodeSize + 1, &jmpBackAddress, sizeof(jmpBackAddress)); // JMP address
    if (!success) {
        LOG_ERROR("Failed to write JMP address to allocated memory page.\n");
        return NULL;
    }

    // Save original bytes before overwriting them with the JMP instruction
    uint8_t *originalBytes = (uint8_t*)malloc(sizeof(uint8_t)*size);
    if (!originalBytes) {
        LOG_ERROR("Failed to allocate memory for original bytes.\n");
        return NULL;
    }

    success = processRead(process, startAddress, originalBytes, size);
    if (!success) {
        LOG_ERROR("Failed to read original bytes from target process.\n");
        free(originalBytes);
        return NULL;
    }

    // Calculate the jump adress to the allocated memory from the hooked function so its not computed on each activation
    uint32_t jmpToHook = pageAddress - (startAddress + 5);

    Hook *hook = (Hook*)malloc(sizeof(Hook));
    if (!hook) {
        LOG_ERROR("Failed to allocate memory for Hook\n");
        return NULL;
    }
    hook->process = process;
    hook->startAddress = startAddress;
    hook->originalBytes = originalBytes;
    hook->originalBytesSize = size;
    hook->jmpToHook = jmpToHook;
    hook->allocatedPageAddress = pageAddress;
    hook->shellCode = shellCode;
    hook->shellCodeSize = shellCodeSize;
    return hook;
}

void hookDestroy(Hook *hook) {
    if (!hook) return;
    if (hook->shellCode) {
        free(hook->shellCode);
        hook->shellCode = NULL;
    }
    if (hook->originalBytes) {
        free(hook->originalBytes);
        hook->originalBytes = NULL;
    }
    free(hook);
}

bool hookIsActivated(Hook *hook) {
    // Read the current bytes from the target process memory
    uint8_t *current = (uint8_t*)malloc(hook->originalBytesSize);
    if (!current) {
        LOG_ERROR("Failed to allocate memory for buffer.\n");
        return false;
    }

    if (!processRead(hook->process, hook->startAddress, current, hook->originalBytesSize)) {
        LOG_ERROR("Failed to read memory.\n");
        free(current);
        return false;
    }

    // If the bytes are identical to the original ones then the hook is deactivated
    if (memcmp(current, hook->originalBytes, hook->originalBytesSize) == 0) {
        free(current);
        return false;
    }

    // Allocate a buffer to build the expected pattern of the active hook (JMP + rel32 + NOP padding)
    uint8_t *expected = (uint8_t*)malloc(hook->originalBytesSize);
    if (!expected) {
        LOG_ERROR("Failed to allocate expected buffer.\n");
        free(current);
        return false;
    }

    // Write JMP opcode and relative offset
    expected[0] = JMP_INSTRUCTION;
    memcpy(&expected[1], &hook->jmpToHook, sizeof(uint32_t));

    // Fill remaining bytes with NOPs (if any)
    for (size_t i = 5; i < hook->originalBytesSize; i++) {
        expected[i] = NOP_INSTRUCTION;
    }

    // Compare the current bytes with the expected hook pattern
    bool active = (memcmp(current, expected, hook->originalBytesSize) == 0);

    free(current);
    free(expected);

    return active;
}

bool hookActivate(Hook *hook) {
    // Change memory protection to allow writing
    uint32_t oldProtect = 0;
    bool success = processVirtualProtect(hook->process, hook->startAddress, hook->originalBytesSize, PAGE_EXECUTE_READWRITE, &oldProtect);
    if (!success) {
        LOG_ERROR("Failed to change memory protection in target process.\n");
        return NULL;
    }

    // Before writing the hook, we need make sure all bytes which will be overwritten are NOPed
    // because if the hook size is not equal with the size of the jmp + address we will have left over bytes.
    if (hook->originalBytesSize > 5) {
		for(uint32_t idx = 0; idx < hook->originalBytesSize; idx++) {
			processWrite(hook->process, hook->startAddress + idx, &NOP_INSTRUCTION, 1); // NOP opcode
        }
	}

    // Write the JMP instruction to the start address
    success = processWrite(hook->process, hook->startAddress, &JMP_INSTRUCTION, sizeof(JMP_INSTRUCTION)); // JMP opcode
    if (!success) {
        LOG_ERROR("Failed to write JMP opcode to target process.\n");
        return NULL;
    }

    // Write the jump address after the jmp instruction in the hooked function
    success = processWrite(hook->process, hook->startAddress + 1, &hook->jmpToHook, sizeof(hook->jmpToHook)); // JMP address
    if (!success) {
        LOG_ERROR("Failed to write JMP address to target process.\n");
        return NULL;
    }

    // Restore the original memory protection
    success = processVirtualProtect(hook->process, hook->startAddress, hook->originalBytesSize, oldProtect, &oldProtect);
    if (!success) {
        LOG_ERROR("Failed to restore memory protection in target process.\n");
        return NULL;
    }
    return true;
}

bool hookDeactivate(Hook *hook) {
    // Change memory protection to allow writing
    uint32_t oldProtect = 0;
    bool success = processVirtualProtect(hook->process, hook->startAddress, hook->originalBytesSize, PAGE_EXECUTE_READWRITE, &oldProtect);
    if (!success) {
        LOG_ERROR("Failed to change memory protection in target process.\n");
        return NULL;
    }

    // Write the original bytes
    success = processWrite(hook->process, hook->startAddress, hook->originalBytes, hook->originalBytesSize);
    if (!success) {
        LOG_ERROR("Failed to write original bytes to target process.\n");
        return NULL;
    }

    // Restore the original memory protection
    success = processVirtualProtect(hook->process, hook->startAddress, hook->originalBytesSize, oldProtect, &oldProtect);
    if (!success) {
        LOG_ERROR("Failed to restore memory protection in target process.\n");
        return NULL;
    }

    return true;
}

uintptr_t hookGetAddress(Hook *hook) {
    if (!hook) return 0;
    return hook->startAddress;
}

uintptr_t hookGetAllocatedPageAddress(Hook *hook) {
    if (!hook) return 0;
    return hook->allocatedPageAddress;
}
