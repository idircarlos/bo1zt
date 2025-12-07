#include "win/process.h"
#include "win/process_internal.h"
#include "logger/logger.h"
#include "win/resources.h"
#include "win/thread.h"
#include "resource_ids.h"
#include <windows.h>
#include <tlhelp32.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define READ_STRING_MAX_SIZE 8192

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
                process->pipe.handle = INVALID_HANDLE_VALUE;
                process->pipe.readEvent = NULL;
                process->pipe.writeEvent = NULL;
                process->pipe.connected = false;
                strcpy(process->executableName, executableName);
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

bool processExec(const char *executableName) {
    LOG_INFO("Trying opening %s\n", executableName);
    HINSTANCE result = ShellExecuteA(NULL, "open", executableName,  NULL, NULL, SW_SHOWNORMAL);
    bool success = (INT_PTR)result > 32; // https://learn.microsoft.com/es-es/windows/win32/api/shellapi/nf-shellapi-shellexecutea
    return success;
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

bool processTerminate(Process *process) {
    if (!processIsRunning(process->executableName)) {
        LOG_ERROR("Cannot terminate %s (PID %d) since is not running anymore\n", process->executableName, process->pid);
        return false;
    }
    if (processIsWindowAttached(process)) {
        LOG_INFO("Trying terminating process %s (PID %d) gracefully...\n", process->executableName, process->pid);
        // Saving these variables since they can be freed/modified by another thread.
        char *executableName = strdup(process->executableName);
        DWORD pid = process->pid;
        PostMessageA(process->windowInfo.hwnd, WM_CLOSE, 0, 0);
        if (WaitForSingleObject(process->handle, 5000) == WAIT_OBJECT_0) {
            LOG_INFO("%s (PID %d) gracefully terminated\n", executableName, pid);
            free(executableName);
            return true;
        }
        free(executableName);
    }
    LOG_WARN("Couldn't terminate process %s (PID %d) gracefully. Get /kill -9'ed\n", process->executableName, process->pid);
    return TerminateProcess(process->handle, 0);
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

void processWaitUntilExits(Process *process) {
    WaitForSingleObject(process->handle, INFINITE);
}

void processClose(Process *process) {
    if (process->handle) {
        CloseHandle(process->handle);
        process->handle = NULL;
        if (process->pipe.readEvent != NULL) {
            CloseHandle(process->pipe.readEvent);
            process->pipe.readEvent = NULL;
        }
        CloseHandle(process->pipe.handle);
        process->pipe.handle = INVALID_HANDLE_VALUE;
        process->pipe.connected = false;
        process->pid = 0;
        process->windowInfo.hwnd = NULL;
    }
    free(process);
}

bool processReadString(Process *process, uint32_t address, char *buffer) {
    char temp[READ_STRING_MAX_SIZE];
    for (size_t i = 0; i < READ_STRING_MAX_SIZE; i++) {
        if (!processRead(process, address + i, &temp[i], 1)) {
            return false;
        }
        if (temp[i] == '\0') {
            memcpy(buffer, temp, i + 1);
            return true;
        }
    }
    LOG_ERROR("Couldn't find null terminator in string at 0x%08X\n", address);
    return false;
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

bool processFreePage(Process *process, uintptr_t address) {
    return VirtualFreeEx(process->handle, (LPVOID)(uintptr_t)address, 0, MEM_RELEASE);
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

HANDLE _processGetHandle(Process *process) {
    return process->handle;
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

bool processInjectDll(Process *process, const char *dllName) {
    if (!process || !process->handle) {
        LOG_ERROR("processInjectDll: Invalid process\n");
        return false;
    }

    if (processHasDll(process, dllName)) {
        LOG_INFO("DLL already injected in process %lu\n", (unsigned long)process->pid);
        return true;
    }

    // Extract DLL from resources to current directory
    if (!resourcesExtractToFile(IDR_CHAT_HOOK_DLL, dllName)) {
        LOG_ERROR("processInjectDll: Failed to extract DLL from resources\n");
        return false;
    }
    
    // Get the full path to the DLL
    char fullDllPath[MAX_PATH];
    if (GetFullPathNameA(dllName, MAX_PATH, fullDllPath, NULL) == 0) {
        LOG_ERROR("processInjectDll: Failed to get full path for DLL\n");
        return false;
    }
    
    // Allocate memory in the target process for the DLL path
    size_t pathLen = strlen(fullDllPath) + 1;
    uintptr_t remoteString;
    
    if (!processAllocatePage(process, pathLen, &remoteString)) {
        LOG_ERROR("processInjectDll: Failed to allocate memory in target process\n");
        return false;
    }

    // Write the DLL path into the target process
    if (!processWrite(process, (uint32_t)remoteString, fullDllPath, pathLen)) {
        LOG_ERROR("processInjectDll: Failed to write DLL path to target process\n");
        processFreePage(process, remoteString);
        return false;
    }
    
    // Get the address of LoadLibraryA
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    if (!kernel32) {
        LOG_ERROR("processInjectDll: Failed to get kernel32.dll handle\n");
        processFreePage(process, remoteString);
        return false;
    }
    
    uintptr_t loadLibraryAddr = (uintptr_t)GetProcAddress(kernel32, "LoadLibraryA");
    if (!loadLibraryAddr) {
        LOG_ERROR("processInjectDll: Failed to get LoadLibraryA address\n");
        processFreePage(process, remoteString);
        return false;
    }
    
    // Create a remote thread to load the DLL
    LOG_INFO("Creating remote thread to inject DLL into process %lu\n", (unsigned long)process->pid);
    Thread *remoteThread = threadCreateRemote(process, loadLibraryAddr, remoteString);
    
    if (!remoteThread) {
        LOG_ERROR("processInjectDll: Failed to create remote thread (Error: %lu)\n", GetLastError());
        processFreePage(process, remoteString);
        return false;
    }
    
    // Wait for the thread to finish
    threadWait(remoteThread, INFINITE);
    
    int exitCode = threadGetExitCode(remoteThread);
    
    threadClose(remoteThread);
    processFreePage(process, remoteString);
    
    if (exitCode == 0) {
        LOG_ERROR("processInjectDll: LoadLibraryA failed in target process\n");
        return false;
    }
    
    LOG_INFO("DLL successfully injected!\n");
    return true;
}

bool processHasDll(Process *process, const char *dllName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, process->pid);
    if (snapshot == INVALID_HANDLE_VALUE)
        return false;

    MODULEENTRY32 me;
    me.dwSize = sizeof(MODULEENTRY32);

    if (Module32First(snapshot, &me)) {
        do {
            if (_stricmp(me.szModule, dllName) == 0 || _stricmp(me.szExePath, dllName) == 0) {
                CloseHandle(snapshot);
                return true;
            }
        } while (Module32Next(snapshot, &me));
    }

    CloseHandle(snapshot);
    return false;
}

void processConnectPipe(Process *process) {
    if (process->pipe.handle != INVALID_HANDLE_VALUE) {
        LOG_WARN("processConnectPipe: Pipe already created\n");
        return;
    }

    HANDLE hPipe = CreateFileA(
        PIPE_NAME,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,  // Match server's overlapped mode
        NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        LOG_ERROR("processConnectPipe: Failed to connect to named pipe. Error: %d\n", GetLastError());
        process->pipe.handle = INVALID_HANDLE_VALUE;
        process->pipe.connected = false;
        return;
    }

    process->pipe.readEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (process->pipe.readEvent == NULL) {
        LOG_ERROR("processConnectPipe: Failed to create read event. Error: %d\n", GetLastError());
        CloseHandle(hPipe);
        process->pipe.handle = INVALID_HANDLE_VALUE;
        process->pipe.connected = false;
        return;
    }

    process->pipe.handle = hPipe;
    process->pipe.connected = true;

    LOG_INFO("Pipe connected successfully!\n");
}

bool processIsPipeConnected(Process *process) {
    return process && process->pipe.handle != INVALID_HANDLE_VALUE && process->pipe.connected;
}

Event processPollFromPipe(Process *process) {
    Event event;
    event.type = EVENT_INVALID;
    
    if (process->pipe.handle == INVALID_HANDLE_VALUE) {
        LOG_ERROR("processPollFromPipe: Pipe not created\n");
        return event;
    }

    OVERLAPPED ov;
    memset(&ov, 0, sizeof(ov));
    ov.hEvent = process->pipe.readEvent;
    ResetEvent(process->pipe.readEvent);

    DWORD bytesRead = 0;
    BOOL result = ReadFile(process->pipe.handle, &event, sizeof(Event), &bytesRead, &ov);
    
    if (!result) {
        DWORD error = GetLastError();
        if (error == ERROR_IO_PENDING) {
            // Wait with short timeout (non-blocking poll)
            DWORD waitResult = WaitForSingleObject(process->pipe.readEvent, PIPE_TIMEOUT_MS);
            if (waitResult == WAIT_TIMEOUT) {
                CancelIo(process->pipe.handle);
                event.type = EVENT_INVALID;
                return event;
            }
            // Get result after completion
            if (!GetOverlappedResult(process->pipe.handle, &ov, &bytesRead, FALSE)) {
                error = GetLastError();
                LOG_ERROR("processPollFromPipe: Overlapped read failed (%d)\n", error);
                if (error == ERROR_BROKEN_PIPE || error == ERROR_PIPE_NOT_CONNECTED) {
                    process->pipe.connected = false;
                }
                return event;
            }
        } else {
            LOG_ERROR("processPollFromPipe: Reading from pipe failed (%d)\n", error);
            if (error == ERROR_BROKEN_PIPE || error == ERROR_PIPE_NOT_CONNECTED) {
                process->pipe.connected = false;
            }
            return event;
        }
    }
    
    if (bytesRead != sizeof(Event)) {
        LOG_ERROR("processPollFromPipe: Incomplete read (expected %zu, got %lu bytes)\n", sizeof(Event), bytesRead);
        event.type = EVENT_INVALID;
        return event;
    }

    return event;
}
