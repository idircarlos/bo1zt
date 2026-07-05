#include "gui/character.h"
#include "gui.h"
#include "client/game.h"
#include "logic/game/character.h"
#include "ui.h"

#include <stdint.h>

#define CHARACTER_NUM 5

// Shared HTTP client
static Client *client;

// Parent Window instance
static uiWindow *parent;

static int shownIndex = -1;

static bool applyingSnapshot = false;

static int characterIndexFromName(const char *name) {
    Character character = characterFromName(name);
    return (character == CHARACTER_INVALID) ? 4 : (int)character;
}

static uiRadioButtons *radioButton1;
static uiRadioButtons *radioButton2;
static uiRadioButtons *radioButton3;
static uiRadioButtons *radioButton4;
static uiRadioButtons *radioButton5;

static uiRadioButtons *charactersRadioButtons[CHARACTER_NUM];

static void onRadioButtonSelect(uiRadioButtons *singleRadioButton, void *data) {
    if (applyingSnapshot) return;
    Character character = (Character)(uintptr_t)data;
    for (int i = 0; i < CHARACTER_NUM; i++) {
        if (charactersRadioButtons[i] != singleRadioButton) {
            uiRadioButtonsSetSelected(charactersRadioButtons[i], -1);
        }
    }

    clientSetGameCharacter(client, characterName(character));
    shownIndex = -1;
    guiPollNow();
}

static uiControl *build(Client *clientInstance, uiWindow *parentInstance) {
    client = clientInstance;
    parent = parentInstance;

    uiGroup *characterGroup = uiNewGroup("Character");
    uiBox *characterBox = uiNewVerticalBox();
    uiBoxSetPadded(characterBox, 1);

    radioButton1 = uiNewRadioButtons();
    radioButton2 = uiNewRadioButtons();
    radioButton3 = uiNewRadioButtons();
    radioButton4 = uiNewRadioButtons();
    radioButton5 = uiNewRadioButtons();

    uiRadioButtonsAppend(radioButton1, "Dempsey");
    uiRadioButtonsAppend(radioButton2, "Nikolai");
    uiRadioButtonsAppend(radioButton3, "Takeo");
    uiRadioButtonsAppend(radioButton4, "Richtofen");
    uiRadioButtonsAppend(radioButton5, "Random");

    charactersRadioButtons[0] = radioButton1;
    charactersRadioButtons[1] = radioButton2;
    charactersRadioButtons[2] = radioButton3;
    charactersRadioButtons[3] = radioButton4;
    charactersRadioButtons[4] = radioButton5;

    uiRadioButtonsSetSelected(radioButton5, 0);

    uiRadioButtonsOnSelected(radioButton1, onRadioButtonSelect, (void*)CHARACTER_DEMPSEY);
    uiRadioButtonsOnSelected(radioButton2, onRadioButtonSelect, (void*)CHARACTER_NIKOLAI);
    uiRadioButtonsOnSelected(radioButton3, onRadioButtonSelect, (void*)CHARACTER_TAKEO);
    uiRadioButtonsOnSelected(radioButton4, onRadioButtonSelect, (void*)CHARACTER_RICHTOFEN);
    uiRadioButtonsOnSelected(radioButton5, onRadioButtonSelect, (void*)CHARACTER_RANDOM);
    
    uiGrid *characterGrid = uiNewGrid();
    uiGridSetPadded(characterGrid, 1);
    
    uiGridAppend(characterGrid, uiControl(radioButton1), 0, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(characterGrid, uiControl(radioButton2), 1, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(characterGrid, uiControl(radioButton3), 0, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(characterGrid, uiControl(radioButton4), 1, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(characterGrid, uiControl(radioButton5), 0, 2, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    uiBoxAppend(characterBox, uiControl(characterGrid), 1);
    uiGroupSetChild(characterGroup, uiControl(characterBox));
    uiGroupSetMargined(characterGroup, 1);
    return uiControl(characterGroup);
}

static void update() {
    const GuiSnapshot *snap = guiGetSnapshot();
    if (!snap->valid || !snap->gameConfigValid) return;

    int idx = characterIndexFromName(snap->gameConfig.character);
    if (idx == shownIndex) return;
    shownIndex = idx;

    applyingSnapshot = true;
    for (int i = 0; i < CHARACTER_NUM; i++) {
        uiRadioButtonsSetSelected(charactersRadioButtons[i], i == idx ? 0 : -1);
    }
    applyingSnapshot = false;
}

UIControlGroup *uiCharacterBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}
