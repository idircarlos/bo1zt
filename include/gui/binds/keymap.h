#ifndef UI_BINDS_KEYMAP_H_
#define UI_BINDS_KEYMAP_H_

#include <stdbool.h>

typedef struct {
    const char *keyName;
    int vkCode;
} KeyMapping;

int keymapGetVKCode(const char *keyName);
const KeyMapping *keymapGetMappings(void);
bool keymapIsModifier(const char *keyName);

#endif // UI_BINDS_KEYMAP_H_
