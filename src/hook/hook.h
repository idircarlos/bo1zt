#ifndef HOOK_H_
#define HOOK_H_

#include "../memory/memory.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct Hook Hook;

Hook* hookCreate(ProcessHandle *ph, uintptr_t start, size_t size, uint8_t *shellCode, size_t shellCodeSize);
void hookDestroy(Hook *hook);
bool hookIsActivated(Hook *hook);
bool hookActivate(Hook *hook);
bool hookDeactivate(Hook *hook);
uintptr_t hookGetAddress(Hook *hook);
uintptr_t hookGetAllocatedPageAddress(Hook *hook);

#endif // HOOK_H_