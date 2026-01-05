/*
* BO1ZT DLL - Main Entry Point
*/

#include <windows.h>
#include <stdbool.h>
#include "utils/Log.h"
#include "ipc/event.h"
#include "ipc/pipe.h"
#include "hooks/Hook.h"
#include "gsc/loader.h"

#define PIPE_MAX_EVENTS 128

bool SendEvent(const Event* ev);

// Global pipe instance
static Pipe pipe = {INVALID_HANDLE_VALUE, NULL, NULL, false};
static CRITICAL_SECTION pipeLock;

// Create the named pipe for trainer communication
static HANDLE CreateTrainerPipeHandle() {
    HANDLE handle = CreateNamedPipeA(
        PIPE_NAME,
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, // Overlapped for async writes
        PIPE_TYPE_MESSAGE |
        PIPE_READMODE_MESSAGE |
        PIPE_WAIT,
        1,
        sizeof(Event)*PIPE_MAX_EVENTS,
        sizeof(Event)*PIPE_MAX_EVENTS,
        0,
        NULL);

    if (handle == INVALID_HANDLE_VALUE) {
        LOG_ERROR("Failed to create named pipe. Error: %d", GetLastError());
    }

    return handle;
}

// Monitor pipe connection and detect disconnections
static void MonitorPipeConnection(void) {    
    EnterCriticalSection(&pipeLock);
    
    if (pipe.handle != INVALID_HANDLE_VALUE && pipe.connected) {
        DWORD bytesAvailable = 0;
        BOOL result = PeekNamedPipe(pipe.handle, NULL, 0, NULL, &bytesAvailable, NULL);
        
        if (!result) {
            DWORD error = GetLastError();
            // Client disconnected
            if (error == ERROR_BROKEN_PIPE || error == ERROR_BAD_PIPE || 
                error == ERROR_PIPE_NOT_CONNECTED || error == ERROR_INVALID_HANDLE ||
                error == ERROR_NO_DATA) {
                LOG_INFO("Pipe disconnection detected (Error: %d). Triggering reconnection...", error);
                pipe.connected = false;
            }
        }
    }
    
    LeaveCriticalSection(&pipeLock);
}

// Thread function to wait for trainer connection
static void WaitForTrainerConnection(void) {
    while (true) {
        LOG_INFO("Waiting for pipe connection...");
        
        EnterCriticalSection(&pipeLock);
        
        // Close old pipe if exists
        if (pipe.handle != INVALID_HANDLE_VALUE) {
            CloseHandle(pipe.handle);
            pipe.handle = INVALID_HANDLE_VALUE;
        }
        
        // Create new pipe
        pipe.handle = CreateTrainerPipeHandle();
        
        if (pipe.handle == INVALID_HANDLE_VALUE) {
            LeaveCriticalSection(&pipeLock);
            Sleep(1000);
            continue;
        }
        
        LeaveCriticalSection(&pipeLock);
        
        // Wait for client connection
        BOOL connected = ConnectNamedPipe(pipe.handle, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        
        if (connected) {
            LOG_INFO("Trainer connected successfully!");
            pipe.connected = true;
            
            // Wait until disconnection is detected by monitor thread
            while (pipe.connected) {
                MonitorPipeConnection();
                Sleep(100);
            }
            
            LOG_INFO("Trainer disconnected, preparing to reconnect...");
        } else {
            LOG_ERROR("Failed to connect named pipe. Error: %d", GetLastError());
            EnterCriticalSection(&pipeLock);
            CloseHandle(pipe.handle);
            pipe.handle = INVALID_HANDLE_VALUE;
            LeaveCriticalSection(&pipeLock);
            Sleep(1000);
        }
    }
}

// Function to send events safely with reconnection support (non-blocking with timeout)
bool SendEvent(const Event* ev) {
    if (!ev) return false;
    
    EnterCriticalSection(&pipeLock);
    
    if (pipe.handle == INVALID_HANDLE_VALUE) {
        LeaveCriticalSection(&pipeLock);
        return false;
    }
    
    OVERLAPPED ov;
    memset(&ov, 0, sizeof(ov));
    ov.hEvent = pipe.writeEvent;
    ResetEvent(pipe.writeEvent);
    
    DWORD bytesWritten = 0;
    BOOL result = WriteFile(pipe.handle, ev, sizeof(Event), &bytesWritten, &ov);
    
    if (!result) {
        DWORD error = GetLastError();
        if (error == ERROR_IO_PENDING) {
            // Wait with timeout
            DWORD waitResult = WaitForSingleObject(pipe.writeEvent, PIPE_TIMEOUT_MS);
            if (waitResult == WAIT_TIMEOUT) {
                LOG_WARN("Write timed out, cancelling...");
                CancelIo(pipe.handle);
                LeaveCriticalSection(&pipeLock);
                return false;
            }
            // Get result after completion
            if (!GetOverlappedResult(pipe.handle, &ov, &bytesWritten, FALSE)) {
                error = GetLastError();
                LOG_ERROR("Failed to send event (overlapped). Error: %d", error);
                pipe.connected = false;
                LeaveCriticalSection(&pipeLock);
                return false;
            }
        } else {
            LOG_ERROR("Failed to send event. Error: %d. Triggering reconnection...", error);
            pipe.connected = false;
            LeaveCriticalSection(&pipeLock);
            return false;
        }
    }
        
    if (bytesWritten != sizeof(Event)) {
        LOG_ERROR("Incomplete write: %lu/%zu bytes", bytesWritten, sizeof(Event));
        LeaveCriticalSection(&pipeLock);
        return false;
    }
    
    LeaveCriticalSection(&pipeLock);
    return true;
}

// Initialize BO1ZT DLL
static bool InitBO1ZT() {
    LOG_INFO("Initializing hooks...");

    InitializeCriticalSection(&pipeLock);
    pipe.writeEvent = CreateEvent(NULL, TRUE, FALSE, NULL);  // Manual reset event for overlapped writes

    // Pipe and reconnection thread
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WaitForTrainerConnection, NULL, 0, NULL);

    // Install all hooks
    HookInstallAll();

    // Initialize GSC loader
    GSCInit();

    return 0;
}

// Entry point for the DLL
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            LogSetModule(hinstDLL);
            LogInit("bo1zt_dll.log");
            LOG_INFO("BO1ZT DLL loaded");
            DisableThreadLibraryCalls(hinstDLL);
            InitBO1ZT();
            break;
            
        case DLL_PROCESS_DETACH:
            DeleteCriticalSection(&pipeLock);
            if (pipe.writeEvent != NULL) {
                CloseHandle(pipe.writeEvent);
            }
            if (pipe.handle != INVALID_HANDLE_VALUE) {
                CloseHandle(pipe.handle);
            }
            LOG_INFO("BO1ZT DLL unloaded");
            break;
    }
    
    return TRUE;
}
