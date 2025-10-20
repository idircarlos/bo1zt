#include "memory.h"
#include "../logger/logger.h"
#include <windows.h>
#include <tlhelp32.h>
#include <string.h>
#include <stdlib.h>

struct ProcessHandle {
    HANDLE handle;
    DWORD pid;
};

ProcessHandle *memoryOpenProcess(const char *executableName) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    ProcessHandle *out = NULL;
    if (snap == INVALID_HANDLE_VALUE) return out;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);
    if (Process32First(snap, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, executableName) == 0) {
                out = (ProcessHandle*)malloc(sizeof(ProcessHandle));
                out->pid = pe.th32ProcessID;
                out->handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
                break;
            }
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
    return out;
}

bool memoryIsProcessRunning(const char *executableName) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);
    bool found = false;
    if (Process32First(snap, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, executableName) == 0) {
                found = true;
                break;
            }
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

void memoryWaitUntilProcessCloses(ProcessHandle *ph) {
    WaitForSingleObject(ph->handle, INFINITE);
}

void memoryCloseProcess(ProcessHandle *ph) {
    if (ph->handle) {
        CloseHandle(ph->handle);
        ph->handle = NULL;
        ph->pid = 0;
    }
    free(ph);
}

bool memoryRead(ProcessHandle *ph, uint32_t address, void *buffer, size_t size) {
    SIZE_T bytesRead;
    return ReadProcessMemory(ph->handle, (LPCVOID)(uintptr_t)address, buffer, size, &bytesRead) && bytesRead == size;
}

bool memoryWrite(ProcessHandle *ph, uint32_t address, const void *buffer, size_t size) {
    SIZE_T bytesWritten;
    return WriteProcessMemory(ph->handle, (LPVOID)(uintptr_t)address, buffer, size, &bytesWritten) && bytesWritten == size;
}

bool memoryVirtualProtect(ProcessHandle *ph, uint32_t address, size_t size, uint32_t protect, uint32_t *oldProtect) {
    return VirtualProtectEx(ph->handle, (LPVOID)(uintptr_t)address, size, protect, (PDWORD)oldProtect);
}

bool memoryAllocatePage(ProcessHandle *ph, size_t size, uintptr_t *address) {
    *address = (uintptr_t)VirtualAllocEx(ph->handle, NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    return address != NULL;
}

bool memoryFindPattern(ProcessHandle *ph, uintptr_t startAddress, size_t regionSize, const uint8_t *pattern, size_t patternSize, uintptr_t *outAddress) {
    if (!ph || !pattern || patternSize == 0 || regionSize == 0 || !outAddress)
        return false;

    uint8_t *buffer = (uint8_t*)malloc(regionSize);
    if (!buffer) {
        LOG_ERROR("memoryFindPattern: sin memoria (regionSize=%zu)", regionSize);
        return false;
    }

    if (!memoryRead(ph, (uint32_t)startAddress, buffer, regionSize)) {
        LOG_ERROR("memoryFindPattern: no se pudo leer la memoria en 0x%08X", (unsigned)startAddress);
        free(buffer);
        return false;
    }

    for (size_t i = 0; i <= regionSize - patternSize; ++i) {
        bool match = true;
        for (size_t j = 0; j < patternSize; ++j) {
            if (pattern[j] != 0x3F && buffer[i + j] != pattern[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            *outAddress = startAddress + i;
            free(buffer);
            return true;
        }
    }

    free(buffer);
    return false;
}
