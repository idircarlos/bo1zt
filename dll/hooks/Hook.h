#ifndef DLL_HOOK_H
#define DLL_HOOK_H

#include <stdbool.h>
#include "../../shared/event.h"

typedef struct Hook {
    const char* name;
    bool (*install)(void);
} Hook;

void HookInstallAll(void);
int HookGetTimestamp(void);
Event HookBuildEvent(EventType type, const char* format, ...);

#endif // DLL_HOOK_H
