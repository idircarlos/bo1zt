#include "gui/player.h"
#include "client/player.h"
#include "logger.h"

// Shared HTTP client
static Client *client;

// Parent Window instance
static uiWindow *parent;

// SimpleCheatName Components (Button + Entry/Spinbox)
static uiButton *nameBtn = NULL;
static uiButton *healthBtn = NULL;
static uiButton *pointsBtn = NULL;
static uiButton *speedBtn = NULL;
static uiButton *headshotsBtn = NULL;
static uiButton *killsBtn = NULL;
static uiEntry *nameEntry = NULL;
static uiSpinbox *healthSpin = NULL;
static uiSpinbox *pointsSpin = NULL;
static uiSpinbox *speedSpin = NULL;
static uiSpinbox *headshotsSpin = NULL;
static uiSpinbox *killsSpin = NULL;

static void onPlayerChangeNameButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    char *name = uiEntryText(nameEntry);
    clientSetPlayerName(client, name);
    uiFreeText(name);
}

static void onPlayerButtonClick(uiButton *button, void *data) {
    (void)button;
    SimpleCheatName simpleCheatName = (SimpleCheatName)(uintptr_t)data;

    switch(simpleCheatName) {
        case SIMPLE_CHEAT_NAME_SET_HEALTH:
            clientSetPlayerHealth(client, uiSpinboxValue(healthSpin));
            break;
        case SIMPLE_CHEAT_NAME_SET_POINTS:
            clientSetPlayerPoints(client, uiSpinboxValue(pointsSpin));
            break;
        case SIMPLE_CHEAT_NAME_SET_SPEED:
            clientSetPlayerMovementSpeed(client, uiSpinboxValue(speedSpin));
            break;
        case SIMPLE_CHEAT_NAME_SET_KILLS:
            clientSetPlayerKills(client, uiSpinboxValue(killsSpin));
            break;
        case SIMPLE_CHEAT_NAME_SET_HEADSHOTS:
            clientSetPlayerHeadshots(client, uiSpinboxValue(headshotsSpin));
            break;
        default:
            LOG_WARN("Player panel received unsupported simple cheat %d", simpleCheatName);
    }
}

static uiControl *build(Client *clientInstance, uiWindow *parentInstance) {
    client = clientInstance;
    parent = parentInstance;
    uiGroup *playerGroup = uiNewGroup("Player");
    uiBox *playerBox = uiNewVerticalBox();
    uiBoxSetPadded(playerBox, 1);

    uiGrid *playerGrid = uiNewGrid();
    uiGridSetPadded(playerGrid, 1);

    nameBtn = uiNewButton("Change Name");
    healthBtn = uiNewButton("Set Health");
    pointsBtn = uiNewButton("Set Points");
    speedBtn = uiNewButton("Set Speed");
    killsBtn = uiNewButton("Set Kills");
    headshotsBtn = uiNewButton("Set Headshots");
    nameEntry = uiNewEntry();
    healthSpin = uiNewSpinbox(0, 999999);
    pointsSpin = uiNewSpinbox(0, 9999999);
    speedSpin = uiNewSpinbox(0, 999999);
    headshotsSpin = uiNewSpinbox(0, 999999);
    killsSpin = uiNewSpinbox(0, 999999);

    uiButtonOnClicked(nameBtn, onPlayerChangeNameButtonClick, (void*)SIMPLE_CHEAT_NAME_CHANGE_NAME);
    uiButtonOnClicked(healthBtn, onPlayerButtonClick, (void*)SIMPLE_CHEAT_NAME_SET_HEALTH);
    uiButtonOnClicked(pointsBtn, onPlayerButtonClick, (void*)SIMPLE_CHEAT_NAME_SET_POINTS);
    uiButtonOnClicked(speedBtn, onPlayerButtonClick, (void*)SIMPLE_CHEAT_NAME_SET_SPEED);
    uiButtonOnClicked(killsBtn, onPlayerButtonClick, (void*)SIMPLE_CHEAT_NAME_SET_KILLS);
    uiButtonOnClicked(headshotsBtn, onPlayerButtonClick, (void*)SIMPLE_CHEAT_NAME_SET_HEADSHOTS);
    
    uiGridAppend(playerGrid, uiControl(nameBtn),      0, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(nameEntry),    1, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(healthBtn),    0, 2, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(healthSpin),   1, 2, 1, 1, 1, uiAlignFill, 1, uiAlignFill);


    uiGridAppend(playerGrid, uiControl(pointsBtn),    0, 3, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(pointsSpin),   1, 3, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(speedBtn),     0, 4, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(speedSpin),    1, 4, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    

    uiGridAppend(playerGrid, uiControl(killsBtn),     0, 5, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(killsSpin),    1, 5, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(headshotsBtn), 0, 6, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(playerGrid, uiControl(headshotsSpin),1, 6, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    uiBoxAppend(playerBox, uiControl(playerGrid), 1);

    uiGroupSetChild(playerGroup, uiControl(playerBox));
    uiGroupSetMargined(playerGroup, 1);
    return uiControl(playerGroup);
}

static void update() {
    // Nothing
}

UIControlGroup *uiPlayerBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}
