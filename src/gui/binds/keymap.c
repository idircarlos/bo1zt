#include "gui/binds/keymap.h"
#include <windows.h>
#include <string.h>
#include <stdbool.h>

static const KeyMapping keyMappings[] = {
    {"ESCAPE", VK_ESCAPE},
    {"F1", VK_F1}, {"F2", VK_F2}, {"F3", VK_F3}, {"F4", VK_F4},
    {"F5", VK_F5}, {"F6", VK_F6}, {"F7", VK_F7}, {"F8", VK_F8},
    {"F9", VK_F9}, {"F10", VK_F10}, {"F11", VK_F11}, {"F12", VK_F12},
    {"`", VK_OEM_3}, {"1", '1'}, {"2", '2'}, {"3", '3'}, {"4", '4'},
    {"5", '5'}, {"6", '6'}, {"7", '7'}, {"8", '8'}, {"9", '9'}, {"0", '0'},
    {"-", VK_OEM_MINUS}, {"=", VK_OEM_PLUS},
    {"TAB", VK_TAB},
    {"Q", 'Q'}, {"W", 'W'}, {"E", 'E'}, {"R", 'R'}, {"T", 'T'},
    {"Y", 'Y'}, {"U", 'U'}, {"I", 'I'}, {"O", 'O'}, {"P", 'P'},
    {"[", VK_OEM_4}, {"]", VK_OEM_6}, {"\\", VK_OEM_5},
    {"CAPS", VK_CAPITAL},
    {"A", 'A'}, {"S", 'S'}, {"D", 'D'}, {"F", 'F'}, {"G", 'G'},
    {"H", 'H'}, {"J", 'J'}, {"K", 'K'}, {"L", 'L'},
    {"SEMICOLON", VK_OEM_1}, {"'", VK_OEM_7}, {"ENTER", VK_RETURN},
    {"SHIFT", VK_SHIFT},
    {"Z", 'Z'}, {"X", 'X'}, {"C", 'C'}, {"V", 'V'}, {"B", 'B'},
    {"N", 'N'}, {"M", 'M'}, {",", VK_OEM_COMMA}, {".", VK_OEM_PERIOD}, {"/", VK_OEM_2},
    {"CTRL", VK_CONTROL}, {"ALT", VK_MENU}, {"SPACE", VK_SPACE},
    {"INS", VK_INSERT}, {"HOME", VK_HOME}, {"PGUP", VK_PRIOR},
    {"DEL", VK_DELETE}, {"END", VK_END}, {"PGDN", VK_NEXT},
    {"UPARROW", VK_UP}, {"DOWNARROW", VK_DOWN},
    {"LEFTARROW", VK_LEFT}, {"RIGHTARROW", VK_RIGHT},
    {"MOUSE1", VK_LBUTTON}, {"MOUSE2", VK_RBUTTON}, {"MOUSE3", VK_MBUTTON},
    {"MOUSE4", VK_XBUTTON1}, {"MOUSE5", VK_XBUTTON2},
    {NULL, 0}
};

int keymapGetVKCode(const char *keyName) {
    for (int i = 0; keyMappings[i].keyName != NULL; i++) {
        if (strcmp(keyMappings[i].keyName, keyName) == 0) {
            return keyMappings[i].vkCode;
        }
    }
    return 0;
}

const KeyMapping *keymapGetMappings(void) {
    return keyMappings;
}

bool keymapIsModifier(const char *keyName) {
    if (keyName == NULL) return false;
    return (strcmp(keyName, "CTRL") == 0 || 
            strcmp(keyName, "ALT") == 0 || 
            strcmp(keyName, "SHIFT") == 0);
}
