#include "process.h"
#include "../logger/logger.h"
#include <windows.h>
#include <tlhelp32.h>
#include <string.h>
#include <stdlib.h>

struct Process {
    HANDLE handle;
    DWORD pid;
};

Process *processOpen(const char *executableName) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    Process *out = NULL;
    if (snap == INVALID_HANDLE_VALUE) return out;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);
    if (Process32First(snap, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, executableName) == 0) {
                out = (Process*)malloc(sizeof(Process));
                out->pid = pe.th32ProcessID;
                out->handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
                break;
            }
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
    return out;
}

bool processIsRunning(const char *executableName) {
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

void processWaitUntilCloses(Process *process) {
    WaitForSingleObject(process->handle, INFINITE);
}

void processClose(Process *process) {
    if (process->handle) {
        CloseHandle(process->handle);
        process->handle = NULL;
        process->pid = 0;
    }
    free(process);
}

bool processRead(Process *process, uint32_t address, void *buffer, size_t size) {
    SIZE_T bytesRead;
    return ReadProcessMemory(process->handle, (LPCVOID)(uintptr_t)address, buffer, size, &bytesRead) && bytesRead == size;
}

bool processWrite(Process *process, uint32_t address, const void *buffer, size_t size) {
    SIZE_T bytesWritten;
    return WriteProcessMemory(process->handle, (LPVOID)(uintptr_t)address, buffer, size, &bytesWritten) && bytesWritten == size;
}

bool processVirtualProtect(Process *process, uint32_t address, size_t size, uint32_t protect, uint32_t *oldProtect) {
    return VirtualProtectEx(process->handle, (LPVOID)(uintptr_t)address, size, protect, (PDWORD)oldProtect);
}

bool processAllocatePage(Process *process, size_t size, uintptr_t *address) {
    *address = (uintptr_t)VirtualAllocEx(process->handle, NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    return address != NULL;
}

bool processFindPattern(Process *process, uintptr_t startAddress, size_t regionSize, const uint8_t *pattern, size_t patternSize, uintptr_t *outAddress) {
    if (!process || !pattern || patternSize == 0 || regionSize == 0 || !outAddress)
        return false;

    uint8_t *buffer = (uint8_t*)malloc(regionSize);
    if (!buffer) {
        LOG_ERROR("processFindPattern: sin memoria (regionSize=%zu)", regionSize);
        return false;
    }

    if (!processRead(process, (uint32_t)startAddress, buffer, regionSize)) {
        LOG_ERROR("processFindPattern: no se pudo leer la memoria en 0x%08X", (unsigned)startAddress);
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
