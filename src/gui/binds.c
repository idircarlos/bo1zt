#include "gui/binds.h"
#include "gui/binds/help.h"
#include "controller.h"
#include "gui/binds/keymap.h"
#include "gui/binds/colors.h"
#include "gui/binds/layout.h"
#include "logger.h"
#include "logic/command.h"
#include "logic/command/manager.h"
#include "controller/controller_internal.h"
#include "resource_ids.h"
#include <ui.h>
#include <windows.h>
#include <string.h>
#include <stdlib.h>

static bool prevKeyStates[256] = {false};

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
    int vkCode;
} KeyBind;

#define MAX_KEYS 100
static KeyBind keyBinds[MAX_KEYS];
static int keyBindCount = 0;

static Controller *controller;
static CommandManager *commandManager;
static uiFlexBox *flexBox;
static uiEntry *entryCommand;
static uiButton *btnReset;
static uiButton *btnSave;
static uiButton *btnHelp;
static uiWindow *parent;
static uiCustomButton *selectedKey = NULL;
static bool hasUnsavedChanges = false;

// Help window
static UIControlGroup *helpControlGroup = NULL;
static uiWindow *helpWindow = NULL;

static void onHelpClick(uiButton *button, void *data);
static void onKeyClick(uiCustomButton *button, void *data);
static void onResetClick(uiButton *button, void *data);
static void onSaveClick(uiButton *button, void *data);
static void onEntryChanged(uiEntry *entry, void *data);
static void init(void);
static void loadBindingsFromConfig(void);
static KeyBind *findKeyBindByButton(uiCustomButton *button);
static void executeCommands(const char *commandString);

static inline bool isValidVKCode(int vkCode) {
    return vkCode > 0 && vkCode < 256;
}

static inline bool hasCommand(const KeyBind *kb) {
    return kb != NULL && kb->command != NULL && kb->command[0] != '\0';
}

static void executeCommands(const char *commandString) {
    if (commandString == NULL || commandString[0] == '\0') return;
    
    const char *ptr = commandString;
    
    // Skip leading whitespace
    while (*ptr == ' ' || *ptr == '\t') ptr++;
    
    while (*ptr != '\0') {
        // Each command should start with '/'
        if (*ptr != '/') {
            ptr++;
            continue;
        }
        
        // Find the end of this command (next '/' or end of string)
        const char *cmdStart = ptr;
        ptr++; // Skip the initial '/'
        
        while (*ptr != '\0' && *ptr != '/') {
            ptr++;
        }
        
        // Extract the command
        size_t cmdLen = ptr - cmdStart;
        char *cmd = (char *)malloc(cmdLen + 1);
        strncpy(cmd, cmdStart, cmdLen);
        cmd[cmdLen] = '\0';
        
        // Trim trailing whitespace
        char *end = cmd + strlen(cmd) - 1;
        while (end > cmd && (*end == ' ' || *end == '\t')) {
            *end = '\0';
            end--;
        }
        
        if (cmd[0] != '\0') {
            Command builtCmd = commandBuild(commandManager, cmd);
            if (builtCmd.name != COMMAND_NONE && builtCmd.name != COMMAND_UNKNOWN) {
                commandHandle(builtCmd);
            }
        }
        
        free(cmd);
    }
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
        keyBinds[keyBindCount].vkCode = keymapGetVKCode(keyName);
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

    uiEntrySetText(entryCommand, kb->command != NULL ? kb->command : "");
    
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

static void loadBindingsFromConfig(void) {
    if (!controller) return;
    
    BindsConfig bindsConfig = controllerGetBindsConfig(controller);

    for (int i = 0; i < keyBindCount; i++) {
        if (keyBinds[i].command != NULL) {
            free(keyBinds[i].command);
            keyBinds[i].command = NULL;
        }
    }
    
    for (int i = 0; i < bindsConfig.bindCount; i++) {
        const char *keyName = bindsConfig.binds[i].keyName;
        const char *command = bindsConfig.binds[i].command;
        
        if (keyName[0] == '\0' || command[0] == '\0') continue;
        
        for (int j = 0; j < keyBindCount; j++) {
            if (strcmp(keyBinds[j].keyName, keyName) == 0) {
                if (keyBinds[j].command != NULL) free(keyBinds[j].command);
                keyBinds[j].command = strdup(command);
            }
        }
    }
    
    for (int i = 0; i < keyBindCount; i++) {
        int vkCode = keyBinds[i].vkCode;
        if (isValidVKCode(vkCode)) {
            prevKeyStates[vkCode] = (GetAsyncKeyState(vkCode) & 0x8000) != 0;
        }
    }
    
    updateAllKeyStates();
    hasUnsavedChanges = false;
}

static void onEntryChanged(uiEntry *entry, void *data) {
    (void)entry; (void)data;
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
    controllerUpdateBindsConfig(controller, &bindsConfig);
    
    for (int i = 0; i < keyBindCount; i++) {
        int vkCode = keyBinds[i].vkCode;
        if (isValidVKCode(vkCode)) {
            prevKeyStates[vkCode] = (GetAsyncKeyState(vkCode) & 0x8000) != 0;
        }
    }
    
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
    uiEntrySetText(entryCommand, "");
    
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
    helpControlGroup = uiBindsHelpBuildControlGroup(commandManager);
    
    helpWindow = uiNewWindow("Commands Help", 500, 400, 0);
    uiControl *helpGroup = helpControlGroup->build(controller, helpWindow);
    
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

static uiControl *build(Controller *controllerInstance, uiWindow *parentInstance) {
    controller = controllerInstance;
    parent = parentInstance;
    keyBindCount = 0;
    commandManager = _controllerGetCommandManager(controller);

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

static void update(void) {
    commandManager = _controllerGetCommandManager(controller);
    // Update command history polling for chat arrow keys
    commandManagerUpdate(commandManager);
    
    if (!controller || !controllerIsGameAttached(controller)) return;
    if (!controllerIsGameWindowFocused(controller)) return;
    if (controllerIsChatOpen(controller) || controllerIsZombiesGamePaused(controller)) return;
    
    bool processedThisFrame[MAX_KEYS] = {false};
    
    for (int i = 0; i < keyBindCount; i++) {
        if (keyBinds[i].command == NULL || keyBinds[i].command[0] == '\0') continue;
        if (processedThisFrame[i]) continue;
        
        int vkCode = keyBinds[i].vkCode;
        if (vkCode == 0) continue;
        
        bool isPressed = (GetAsyncKeyState(vkCode) & 0x8000) != 0;
        
        if (isPressed && !prevKeyStates[vkCode]) {
            executeCommands(keyBinds[i].command);
            LOG_DEBUG("Keybind executed: %s -> %s", keyBinds[i].keyName, keyBinds[i].command);
            
            if (keymapIsModifier(keyBinds[i].keyName)) {
                int pairIndex = getModifierPairIndex(&keyBinds[i]);
                if (pairIndex >= 0) processedThisFrame[pairIndex] = true;
            }
        }
        
        prevKeyStates[vkCode] = isPressed;
    }
}

static void init(void) {
    loadBindingsFromConfig();
    uiControlDisable(uiControl(btnReset));
    uiControlDisable(uiControl(btnSave));
}

UIControlGroup *uiBindsBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}

bool uiBindsIsSavable() {
    return hasUnsavedChanges;
}

void uiBindsReset() {
    selectedKey = NULL;
    uiEntrySetText(entryCommand, "");
    loadBindingsFromConfig();
    uiControlDisable(uiControl(btnReset));
    uiControlDisable(uiControl(btnSave));
}
