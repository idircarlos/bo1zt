#include "gui/binds.h"
#include "gui/binds/help.h"
#include "client/binds.h"
#include "logic/bind/keymap.h"
#include "gui/binds/colors.h"
#include "gui/binds/layout.h"
#include "logger.h"
#include "logic/config.h"
#include "resource_ids.h"
#include <ui.h>
#include <windows.h>
#include <string.h>
#include <stdlib.h>

typedef enum {
    KEYBIND_STATE_EMPTY,
    KEYBIND_STATE_ASSIGNED,
    KEYBIND_STATE_SELECTED
} KeyBindState;

typedef struct {
    uiCustomButton *button;
    const char *keyName;
    char *command;
    KeyBindState state;
    bool modifierProcessed;
} KeyBind;

#define MAX_KEYS 100
static KeyBind keyBinds[MAX_KEYS];
static int keyBindCount = 0;

#define BINDS_POLL_INTERVAL_MS 250
static ULONGLONG lastPoll = 0;

static Client *client = NULL;
static uiFlexBox *flexBox;
static uiEntry *entryCommand;
static uiButton *btnReset;
static uiButton *btnSave;
static uiButton *btnHelp;
static uiWindow *parent;
static uiCustomButton *selectedKey = NULL;
static bool hasUnsavedChanges = false;
static bool syncingEntry = false;

// Help window
static uiWindow *helpWindow = NULL;

static void onHelpClick(uiButton *button, void *data);
static void onKeyClick(uiCustomButton *button, void *data);
static void onResetClick(uiButton *button, void *data);
static void onSaveClick(uiButton *button, void *data);
static void onEntryChanged(uiEntry *entry, void *data);
static void init(void);
static void loadBindingsFromClient(void);
static KeyBind *findKeyBindByButton(uiCustomButton *button);

static inline bool hasCommand(const KeyBind *kb) {
    return kb != NULL && kb->command != NULL && kb->command[0] != '\0';
}

static void setCommandText(const char *text) {
    syncingEntry = true;
    uiEntrySetText(entryCommand, text != NULL ? text : "");
    syncingEntry = false;
}

static void updateKeyState(KeyBind *kb) {
    if (kb == NULL || kb->button == NULL) return;

    if (selectedKey == kb->button) {
        kb->state = KEYBIND_STATE_SELECTED;
    } else if (kb->command != NULL && kb->command[0] != '\0') {
        kb->state = KEYBIND_STATE_ASSIGNED;
    } else {
        kb->state = KEYBIND_STATE_EMPTY;
    }
}

static void updateKeyColor(KeyBind *kb) {
    if (kb == NULL || kb->button == NULL) return;

    switch (kb->state) {
        case KEYBIND_STATE_EMPTY:
            uiCustomButtonSetBackgroundColor(kb->button, COLOR_EMPTY.r, COLOR_EMPTY.g, COLOR_EMPTY.b);
            uiCustomButtonSetHoverColor(kb->button, COLOR_HOVER_EMPTY.r, COLOR_HOVER_EMPTY.g, COLOR_HOVER_EMPTY.b);
            break;
        case KEYBIND_STATE_ASSIGNED:
            uiCustomButtonSetBackgroundColor(kb->button, COLOR_ASSIGNED.r, COLOR_ASSIGNED.g, COLOR_ASSIGNED.b);
            uiCustomButtonSetHoverColor(kb->button, COLOR_HOVER_ASSIGNED.r, COLOR_HOVER_ASSIGNED.g, COLOR_HOVER_ASSIGNED.b);
            break;
        case KEYBIND_STATE_SELECTED:
            uiCustomButtonSetBackgroundColor(kb->button, COLOR_SELECTED.r, COLOR_SELECTED.g, COLOR_SELECTED.b);
            uiCustomButtonSetHoverColor(kb->button, COLOR_SELECTED.r, COLOR_SELECTED.g, COLOR_SELECTED.b);
            break;
    }

    uiCustomButtonSetPressedColor(kb->button, COLOR_PRESSED.r, COLOR_PRESSED.g, COLOR_PRESSED.b);
    uiCustomButtonSetBorderColor(kb->button, COLOR_BORDER.r, COLOR_BORDER.g, COLOR_BORDER.b);
}

static void updateAllKeyStates(void) {
    for (int i = 0; i < keyBindCount; i++) {
        updateKeyState(&keyBinds[i]);
        updateKeyColor(&keyBinds[i]);
    }
}

static int getModifierPairIndex(KeyBind *kb) {
    if (kb == NULL || !keymapIsModifier(kb->keyName)) return -1;

    for (int i = 0; i < keyBindCount; i++) {
        if (keyBinds[i].button != kb->button &&
            strcmp(keyBinds[i].keyName, kb->keyName) == 0) {
            return i;
        }
    }
    return -1;
}

static KeyBind *getModifierPair(KeyBind *kb) {
    int pairIndex = getModifierPairIndex(kb);
    return (pairIndex >= 0) ? &keyBinds[pairIndex] : NULL;
}

static void syncModifierCommand(KeyBind *kb, const char *command) {
    if (kb == NULL || !keymapIsModifier(kb->keyName)) return;

    KeyBind *pair = getModifierPair(kb);
    if (pair != NULL) {
        if (pair->command != NULL) {
            free(pair->command);
            pair->command = NULL;
        }
        if (command != NULL && command[0] != '\0') {
            pair->command = strdup(command);
        }
        updateKeyState(pair);
        updateKeyColor(pair);
    }
}

static uiCustomButton *buildKey(const char *label, const char *keyName, int x, int y, int w, int h) {
    uiCustomButton *btn = uiNewCustomButton(label);
    uiCustomButtonSetFlat(btn, 1);
    uiFlexBoxAppend(flexBox, uiControl(btn), x, y, w, h);

    if (keyBindCount < MAX_KEYS) {
        keyBinds[keyBindCount].button = btn;
        keyBinds[keyBindCount].keyName = keyName;
        keyBinds[keyBindCount].command = NULL;
        keyBinds[keyBindCount].state = KEYBIND_STATE_EMPTY;
        updateKeyColor(&keyBinds[keyBindCount]);
        uiCustomButtonOnClicked(btn, onKeyClick, &keyBinds[keyBindCount]);
        keyBindCount++;
    }

    return btn;
}

static void updateKeyBindCommand(void) {
    if (selectedKey == NULL) return;

    KeyBind *kb = findKeyBindByButton(selectedKey);
    if (kb == NULL) return;

    char *command = uiEntryText(entryCommand);
    const char *oldCommand = kb->command;
    bool commandChanged = false;

    if ((oldCommand == NULL && command != NULL && strlen(command) > 0) ||
        (oldCommand != NULL && command == NULL) ||
        (oldCommand != NULL && command != NULL && strcmp(oldCommand, command) != 0)) {
        commandChanged = true;
    }

    if (kb->command != NULL) {
        free(kb->command);
        kb->command = NULL;
    }

    if (command != NULL && strlen(command) > 0) {
        kb->command = strdup(command);
    }

    syncModifierCommand(kb, command);
    uiFreeText(command);

    if (commandChanged) {
        hasUnsavedChanges = true;
        uiControlEnable(uiControl(btnReset));
        uiControlEnable(uiControl(btnSave));
    }
}

static void onKeyClick(uiCustomButton *button, void *data) {
    KeyBind *kb = (KeyBind *)data;

    if (selectedKey != NULL && selectedKey != button) {
        updateKeyBindCommand();

        KeyBind *prevKb = findKeyBindByButton(selectedKey);
        selectedKey = NULL;  // Clear before updating state

        if (prevKb != NULL) {
            KeyBind *prevPair = getModifierPair(prevKb);
            if (prevPair != NULL) {
                updateKeyState(prevPair);
                updateKeyColor(prevPair);
            }
            updateKeyState(prevKb);
            updateKeyColor(prevKb);
        }
    }

    selectedKey = button;
    kb->state = KEYBIND_STATE_SELECTED;
    updateKeyColor(kb);

    KeyBind *pair = getModifierPair(kb);
    if (pair != NULL) {
        pair->state = KEYBIND_STATE_SELECTED;
        updateKeyColor(pair);
    }

    setCommandText(kb->command);

    if (hasCommand(kb)) {
        uiControlEnable(uiControl(btnReset));
    } else {
        uiControlDisable(uiControl(btnReset));
    }
}

static KeyBind *findKeyBindByButton(uiCustomButton *button) {
    for (int i = 0; i < keyBindCount; i++) {
        if (keyBinds[i].button == button) {
            return &keyBinds[i];
        }
    }
    return NULL;
}

static void buildBindsConfigFromUI(BindsConfig *bindsConfig) {
    bindsConfig->bindCount = 0;

    for (int i = 0; i < keyBindCount; i++) {
        keyBinds[i].modifierProcessed = false;
    }

    for (int i = 0; i < keyBindCount && bindsConfig->bindCount < MAX_BINDS; i++) {
        if (keyBinds[i].command == NULL || keyBinds[i].command[0] == '\0') continue;

        if (keymapIsModifier(keyBinds[i].keyName)) {
            if (keyBinds[i].modifierProcessed) continue;

            int pairIndex = getModifierPairIndex(&keyBinds[i]);
            if (pairIndex >= 0) {
                keyBinds[pairIndex].modifierProcessed = true;
            }
        }

        strncpy(bindsConfig->binds[bindsConfig->bindCount].keyName,
                keyBinds[i].keyName, MAX_KEY_NAME_LENGTH - 1);
        bindsConfig->binds[bindsConfig->bindCount].keyName[MAX_KEY_NAME_LENGTH - 1] = '\0';

        strncpy(bindsConfig->binds[bindsConfig->bindCount].command,
                keyBinds[i].command, MAX_COMMAND_LENGTH - 1);
        bindsConfig->binds[bindsConfig->bindCount].command[MAX_COMMAND_LENGTH - 1] = '\0';

        bindsConfig->bindCount++;
    }
}

static const char *commandForKeyInConfig(const BindsConfig *cfg, const char *keyName) {
    for (int i = 0; i < cfg->bindCount; i++) {
        if (strcmp(cfg->binds[i].keyName, keyName) == 0) {
            return cfg->binds[i].command;
        }
    }
    return NULL;
}

static void applyBindsConfig(const BindsConfig *bindsConfig) {
    for (int i = 0; i < keyBindCount; i++) {
        if (keyBinds[i].command != NULL) {
            free(keyBinds[i].command);
            keyBinds[i].command = NULL;
        }
    }

    for (int i = 0; i < bindsConfig->bindCount; i++) {
        const char *keyName = bindsConfig->binds[i].keyName;
        const char *command = bindsConfig->binds[i].command;

        if (keyName[0] == '\0' || command[0] == '\0') continue;

        for (int j = 0; j < keyBindCount; j++) {
            if (strcmp(keyBinds[j].keyName, keyName) == 0) {
                if (keyBinds[j].command != NULL) free(keyBinds[j].command);
                keyBinds[j].command = strdup(command);
            }
        }
    }

    updateAllKeyStates();
}

static bool bindsConfigDiffersFromUI(const BindsConfig *cfg) {
    for (int i = 0; i < keyBindCount; i++) {
        const char *cfgCmd = commandForKeyInConfig(cfg, keyBinds[i].keyName);
        const char *uiCmd = keyBinds[i].command;
        bool cfgEmpty = (cfgCmd == NULL || cfgCmd[0] == '\0');
        bool uiEmpty = (uiCmd == NULL || uiCmd[0] == '\0');
        if (cfgEmpty != uiEmpty) return true;
        if (!cfgEmpty && strcmp(cfgCmd, uiCmd) != 0) return true;
    }
    return false;
}

static void loadBindingsFromClient(void) {
    BindsConfig bindsConfig;
    if (clientGetBinds(client, &bindsConfig) != CLIENT_OK) {
        bindsConfig.bindCount = 0;
    }
    applyBindsConfig(&bindsConfig);
    hasUnsavedChanges = false;
}

static void onEntryChanged(uiEntry *entry, void *data) {
    (void)entry; (void)data;
    if (syncingEntry) return;
    if (selectedKey == NULL) return;

    KeyBind *kb = findKeyBindByButton(selectedKey);
    char *text = uiEntryText(entryCommand);
    bool hasText = text != NULL && text[0] != '\0';
    uiFreeText(text);

    if (hasCommand(kb) || hasText) {
        uiControlEnable(uiControl(btnReset));
    } else {
        uiControlDisable(uiControl(btnReset));
    }

    hasUnsavedChanges = true;
    uiControlEnable(uiControl(btnSave));
}

static void onSaveClick(uiButton *button, void *data) {
    (void)button; (void)data;

    updateKeyBindCommand();
    updateAllKeyStates();

    BindsConfig bindsConfig;
    buildBindsConfigFromUI(&bindsConfig);
    clientSetBinds(client, &bindsConfig);

    hasUnsavedChanges = false;
    uiControlDisable(uiControl(btnSave));

    if (selectedKey != NULL) {
        KeyBind *kb = findKeyBindByButton(selectedKey);
        if (hasCommand(kb)) {
            uiControlEnable(uiControl(btnReset));
        } else {
            uiControlDisable(uiControl(btnReset));
        }
    } else {
        uiControlDisable(uiControl(btnReset));
    }
    LOG_INFO("All keybindings saved!");
}

static void onResetClick(uiButton *button, void *data) {
    (void)button; (void)data;

    if (selectedKey == NULL) return;

    KeyBind *kb = findKeyBindByButton(selectedKey);
    if (kb == NULL) return;

    if (kb->command != NULL) {
        free(kb->command);
        kb->command = NULL;
    }

    syncModifierCommand(kb, NULL);
    setCommandText(NULL);

    updateKeyState(kb);
    updateKeyColor(kb);

    hasUnsavedChanges = true;
    uiControlDisable(uiControl(btnReset));
    uiControlEnable(uiControl(btnSave));
    LOG_INFO("Command cleared for key: %s", kb->keyName);
}

static int onHelpWindowClose(uiWindow *w, void *data) {
    (void)data;
    uiControlHide(uiControl(w));
    return 0;
}

static void buildHelp(void) {
    helpWindow = uiNewWindow("Commands Help", 500, 400, 0);
    uiControl *helpGroup = uiBindsHelpBuild(client, helpWindow);

    uiWindowOnClosing(helpWindow, onHelpWindowClose, NULL);
    uiWindowSetMargined(helpWindow, 1);
    uiWindowSetChild(helpWindow, uiControl(helpGroup));
    uiWindowSetResizeable(helpWindow, false);
    uiWindowSetMargined(helpWindow, true);
    uiWindowSetIcon(helpWindow, IDI_ICON1);
}

static void onHelpClick(uiButton *button, void *data) {
    (void)button; (void)data;
    uiControlShow(uiControl(helpWindow));
}

uiControl *uiBindsBuild(Client *clientInstance, uiWindow *parentInstance) {
    client = clientInstance;
    parent = parentInstance;
    keyBindCount = 0;

    uiBox *outerBox = uiNewVerticalBox();
    uiBoxSetPadded(outerBox, 1);
    flexBox = uiNewFlexBox();

    for (int i = 0; keyboardLayout[i].label != NULL; i++) {
        const KeyLayout *key = &keyboardLayout[i];
        buildKey(key->label, key->keyName, key->x, key->y, key->w, key->h);
    }

    uiLabel *lblCommand = uiNewLabel("Command(s):");
    uiFlexBoxAppend(flexBox, uiControl(lblCommand), 18, 314, 72, 20);

    entryCommand = uiNewEntry();
    uiEntrySetPlaceholder(entryCommand, "/perk add qr jg sc dt /give tg ray mk (You can add multiple commands separated by spaces)");
    uiEntryOnChanged(entryCommand, onEntryChanged, NULL);
    uiFlexBoxAppend(flexBox, uiControl(entryCommand), 96, 311, 646, 20);

    btnReset = uiNewButton("Reset");
    uiButtonOnClicked(btnReset, onResetClick, NULL);
    uiFlexBoxAppend(flexBox, uiControl(btnReset), 956, 309, 85, 26);

    btnSave = uiNewButton("Save");
    uiButtonOnClicked(btnSave, onSaveClick, NULL);
    uiFlexBoxAppend(flexBox, uiControl(btnSave), 1048, 309, 85, 26);

    btnHelp = uiNewButton("Help");
    uiButtonOnClicked(btnHelp, onHelpClick, NULL);
    uiFlexBoxAppend(flexBox, uiControl(btnHelp), 756, 309, 85, 26);

    uiBoxAppend(outerBox, uiControl(flexBox), 1);
    init();
    buildHelp();

    return uiControl(outerBox);
}

static void init(void) {
    loadBindingsFromClient();
    uiControlDisable(uiControl(btnReset));
    uiControlDisable(uiControl(btnSave));
}

bool uiBindsIsSavable() {
    return hasUnsavedChanges;
}

void uiBindsReset() {
    selectedKey = NULL;
    setCommandText(NULL);
    loadBindingsFromClient();
    uiControlDisable(uiControl(btnReset));
    uiControlDisable(uiControl(btnSave));
}

void uiBindsUpdate(void) {
    if (parent == NULL || !uiControlVisible(uiControl(parent))) return;
    if (hasUnsavedChanges) return;

    ULONGLONG now = GetTickCount64();
    if (lastPoll != 0 && now - lastPoll < BINDS_POLL_INTERVAL_MS) return;
    lastPoll = now;

    BindsConfig fresh;
    if (clientGetBinds(client, &fresh) != CLIENT_OK) return;
    if (!bindsConfigDiffersFromUI(&fresh)) return;

    applyBindsConfig(&fresh);

    if (selectedKey != NULL) {
        KeyBind *kb = findKeyBindByButton(selectedKey);
        if (kb != NULL) {
            KeyBind *pair = getModifierPair(kb);
            if (pair != NULL) {
                pair->state = KEYBIND_STATE_SELECTED;
                updateKeyColor(pair);
            }
            setCommandText(kb->command);
        }
    }
}
