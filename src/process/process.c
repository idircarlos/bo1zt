#include "process.h"
#include "../logger/logger.h"
#include <windows.h>
#include <tlhelp32.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    HWND hwnd;
    char *windowTitle;
    LONG_PTR originalStyle;
    LONG_PTR originalExStyle;
    bool hasSavedStyle;
} WindowInfo;

struct Process {
    HANDLE handle;
    DWORD pid;
    WindowInfo windowInfo;
};

static BOOL CALLBACK _EnumWindowsProc(HWND hWnd, LPARAM lParam);
static bool _tryMakeBorderless(Process *process);
static bool _tryMakeNonBorderless(Process *process);

Process *processOpen(const char *executableName) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    Process *process = NULL;
    if (snap == INVALID_HANDLE_VALUE) return process;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);
    if (Process32First(snap, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, executableName) == 0) {
                process = (Process*)malloc(sizeof(Process));
                process->pid = pe.th32ProcessID;
                process->handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
                WindowInfo windowInfo = { .hwnd = NULL, .windowTitle = NULL, .originalStyle = 0, .originalExStyle = 0, .hasSavedStyle = false };
                process->windowInfo = windowInfo;
                break;
            }
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
    return process;
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

bool processIsWindowAttached(Process *process) {
    return process->windowInfo.hwnd != NULL;
}

bool processTryAttachWindow(Process *process, const char *windowTitle) {
    process->windowInfo.windowTitle = (char*)malloc(sizeof(char)*(strlen(windowTitle) + 1));
    strcpy(process->windowInfo.windowTitle, windowTitle);
    EnumWindows(_EnumWindowsProc, (LPARAM)process);
    return process->windowInfo.hwnd != NULL;
}

bool processIsBorderless(Process *process) {
    HWND hwnd = process->windowInfo.hwnd;
    if (!IsWindow(hwnd)) return false;

    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);

    bool hasBorderFlags = (style & (WS_CAPTION | WS_THICKFRAME)) != 0;
    bool hasExBorderFlags = (exStyle & (WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE)) != 0;

    return !hasBorderFlags && !hasExBorderFlags;
}

bool processMakeBorderless(Process *process, bool enabled) {
    HWND hwnd = process->windowInfo.hwnd;
    if (hwnd == NULL) {
        LOG_ERROR("Couldn't find window for pid %u\n", process->pid);
        return false;
    }

    if (enabled) {
        return _tryMakeBorderless(process);
    }
    return _tryMakeNonBorderless(process);
}

void processWaitUntilCloses(Process *process) {
    WaitForSingleObject(process->handle, INFINITE);
}

void processClose(Process *process) {
    if (process->handle) {
        process->handle = NULL;
        process->pid = 0;
        process->windowInfo.hwnd = NULL;
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

static BOOL CALLBACK _EnumWindowsProc(HWND hWnd, LPARAM lParam) {
    Process *process = (Process*)lParam;
    DWORD windowPid = 0;
    GetWindowThreadProcessId(hWnd, &windowPid);

    // Find main window visible and without owner
    if (windowPid == process->pid) {
        if (IsWindowVisible(hWnd) && GetWindow(hWnd, GW_OWNER) == NULL) {
            char title[256];
            int length = GetWindowTextA(hWnd, title, sizeof(title));
            if (length == 0) {
                LOG_ERROR("Couldn't retrieve the process window title");
                return TRUE;
            }
            // We need to ensure that we attach the Main Game Window and not any other like Warnings for not exiting the game correctly. Only check for the firsts chars.
            // See: https://www.reddit.com/r/Warzone/comments/1lxoef1/accidentally_booted_game_into_safe_mode_help/?tl=es-419
            if (strncmp(title, process->windowInfo.windowTitle, strlen(process->windowInfo.windowTitle)) == 0) {
                process->windowInfo.hwnd = hWnd;
                return FALSE; // We found the window, stop enumeration
            }
        }
    }
    return TRUE; // Continue
}

static bool _tryMakeBorderless(Process *process) {
    HWND hwnd = process->windowInfo.hwnd;
    if (!IsWindow(hwnd)) {
        LOG_ERROR("MakeWindowBorderless: hwnd no válido\n");
        return false;
    }

    // Save window styles to restore non-borderless mode
    if (!process->windowInfo.hasSavedStyle) {
        process->windowInfo.originalStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
        process->windowInfo.originalExStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        process->windowInfo.hasSavedStyle = true;
    }

    // Remove window styles to get the borderless appearnce
    LONG_PTR style = process->windowInfo.originalStyle;
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
    SetWindowLongPtr(hwnd, GWL_STYLE, style);

    LONG_PTR exStyle = process->windowInfo.originalExStyle;
    exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);

    // Get monitor information
    MONITORINFO mi = { };
    mi.cbSize = sizeof(mi);
    HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    GetMonitorInfo(hMon, &mi);
    RECT r = mi.rcMonitor;

    // Expand the window to fit the entire monitor
    return SetWindowPos(hwnd, HWND_TOP,
                        r.left, r.top,
                        r.right - r.left,
                        r.bottom - r.top,
                        SWP_FRAMECHANGED | SWP_SHOWWINDOW);
}

static bool _tryMakeNonBorderless(Process *process) {
    HWND hwnd = process->windowInfo.hwnd;
    if (!IsWindow(hwnd)) return false;

    if (!process->windowInfo.hasSavedStyle) {
        LOG_WARN("There are no saved window styles\n");
        return false;
    }

    // Restore saved window styles for non-borderless
    SetWindowLongPtr(hwnd, GWL_STYLE, process->windowInfo.originalStyle);
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, process->windowInfo.originalExStyle);

    return SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
}
