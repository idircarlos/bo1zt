#include "gui/character.h"
#include "logic/game/character.h"
#include "logic/cheat/manager.h"
#include "logic/cheat/manager/actions.h"
#include "logic/config.h"
#include "ui.h"

#define CHARACTER_NUM 5

// Controller instance
static Controller *controller;

// Parent Window instance
static uiWindow *parent;

static uiRadioButtons *radioButton1;
static uiRadioButtons *radioButton2;
static uiRadioButtons *radioButton3;
static uiRadioButtons *radioButton4;
static uiRadioButtons *radioButton5;

static uiRadioButtons *charactersRadioButtons[CHARACTER_NUM];

static void onRadioButtonSelect(uiRadioButtons *singleRadioButton, void *data) {
    Character character = (Character)(uintptr_t)data;
    for (int i = 0; i < CHARACTER_NUM; i++) {
        if (charactersRadioButtons[i] != singleRadioButton) {
            uiRadioButtonsSetSelected(charactersRadioButtons[i], -1);
        }
    }
    
    int characterValue = (int)character;
    CheatManager *cheatManager = controllerGetCheatManager(controller);
    cheatManagerSetValue(cheatManager, SIMPLE_CHEAT_NAME_CHARACTER, &characterValue);
}

static uiControl *build(Controller *controllerInstance, uiWindow *parentInstance) {
    controller = controllerInstance;
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

    Config *config = controllerGetConfig(controller);
    int savedCharacter = config ? config->game.character : CHARACTER_RANDOM;
    if (savedCharacter >= 0 && savedCharacter < CHARACTER_NUM) {
        uiRadioButtonsSetSelected(charactersRadioButtons[savedCharacter], 0);
    } else {
        uiRadioButtonsSetSelected(radioButton5, 0);
    }

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
}

UIControlGroup *uiCharacterBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}
