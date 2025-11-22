#ifndef PROCESS_INTERNAL_H_
#define PROCESS_INTERNAL_H_

#include <windows.h>
#include "process.h"

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
    char executableName[256];
    WindowInfo windowInfo;
};

HANDLE _processGetHandle(Process *process);

#endif // PROCESS_INTERNAL_H_