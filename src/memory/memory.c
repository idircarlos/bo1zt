#include "memory.h"
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
    BOOL found = FALSE;
    if (Process32First(snap, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, executableName) == 0) {
                out = (ProcessHandle*)malloc(sizeof(ProcessHandle));
                out->pid = pe.th32ProcessID;
                out->handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
                found = (out->handle != NULL);
                break;
            }
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
    return out;
}

void memoryCloseProcess(ProcessHandle *ph) {
    if (ph->handle) {
        CloseHandle(ph->handle);
        ph->handle = NULL;
        ph->pid = 0;
    }
}

bool memoryRead(ProcessHandle *ph, uint32_t address, void *buffer, size_t size) {
    SIZE_T bytesRead;
    return ReadProcessMemory(ph->handle, (LPCVOID)(uintptr_t)address, buffer, size, &bytesRead) && bytesRead == size;
}

bool memoryWrite(ProcessHandle *ph, uint32_t address, const void *buffer, size_t size) {
    SIZE_T bytesWritten;
    return WriteProcessMemory(ph->handle, (LPVOID)(uintptr_t)address, buffer, size, &bytesWritten) && bytesWritten == size;
}