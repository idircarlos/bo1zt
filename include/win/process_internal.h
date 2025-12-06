#ifndef PROCESS_INTERNAL_H_
#define PROCESS_INTERNAL_H_

#include <windows.h>
#include "win/process.h"
#include "ipc/pipe.h"

typedef struct {
    HWND hwnd;
    char *windowTitle;
    LONG_PTR originalStyle;
    LONG_PTR originalExStyle;
    bool hasSavedStyle;
} WindowInfo;

struct Process {
    HANDLE handle;
    Pipe pipe;
    DWORD pid;
    char executableName[256];
    WindowInfo windowInfo;
};

HANDLE _processGetHandle(Process *process);

#endif // PROCESS_INTERNAL_H_
