#ifndef SHARED_PIPE_H_
#define SHARED_PIPE_H_

#include <stdbool.h>
#include <windows.h>

#define PIPE_NAME "\\\\.\\pipe\\BO1ZT_EventPipe"
#define PIPE_TIMEOUT_MS 50

typedef struct Pipe {
    HANDLE handle;
    HANDLE readEvent;   // Event for overlapped reads (trainer side)
    HANDLE writeEvent;  // Event for overlapped writes (DLL side)
    bool connected;
} Pipe;

#endif // SHARED_PIPE_H_
