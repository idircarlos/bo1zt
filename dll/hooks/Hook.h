#ifndef DLL_HOOK_H
#define DLL_HOOK_H

#include <stdbool.h>

typedef struct Hook {
    const char* name;
    bool (*install)(void);
} Hook;

void HookInstallAll(void);
unsigned int HookGetTimestamp(void);

#endif // DLL_HOOK_H
