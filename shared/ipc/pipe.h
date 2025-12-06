#ifndef SHARED_PIPE_H_
#define SHARED_PIPE_H_

#include <stdbool.h>
#include <windows.h>

#define PIPE_NAME "\\\\.\\pipe\\BO1ZT_EventPipe"

typedef struct Pipe {
    HANDLE handle;
    bool connected;
} Pipe;

#endif // SHARED_PIPE_H_
