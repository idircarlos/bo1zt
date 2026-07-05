#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <ui.h>
#include "gui.h"
#include "logger.h"
#include "resource_ids.h"
#include "client/cheats.h"
#include "client/state.h"
#include "client/game.h"
#include "client/graphics.h"
#include "gui/player.h"
#include "gui/character.h"
#include "gui/weapons.h"
#include "gui/teleport.h"
#include "gui/hacks.h"
#include "gui/graphics.h"
#include "gui/game.h"
#include "gui/about.h"

#define WINDOW_WIDTH 100
#define WINDOW_HEIGHT 540
#define UI_CONTROL_GROUP_SIZE 8

#define GUI_POLL_INTERVAL_MS 250
static UIControlGroup *controlGroups[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
static Client *client = NULL;
static int clientPort = 0;
static GuiSnapshot snapshot;

// UI Elements
static uiWindow *window = NULL;

// Handlers
static int onClosing(uiWindow *window, void *data) {
    (void)window;
    (void)data;
    uiQuit();
    exit(0);
}

static int onTimerUpdate(void *data) {
    (void)data;
    guiUpdate();
    return 1;
}

static ULONGLONG lastPoll = 0;

static void guiPoll(void) {
    if (!client) return;

    snapshot.statusValid = (clientGetGameStatus(client, &snapshot.status) == CLIENT_OK);
    snapshot.stateValid = (clientGetState(client, &snapshot.state) == CLIENT_OK);
    snapshot.graphicsValid = (clientGetGraphics(client, &snapshot.graphics) == CLIENT_OK);
    snapshot.gameConfigValid = (clientGetGameConfig(client, &snapshot.gameConfig) == CLIENT_OK);

    int count = 0;
    if (clientGetCheats(client, snapshot.cheats, 32, &count) == CLIENT_OK) {
        snapshot.cheatCount = count;
        snapshot.cheatsValid = true;
    } else {
        snapshot.cheatsValid = false;
    }
    snapshot.valid = true;
}

void guiPollNow(void) {
    guiPoll();
    lastPoll = GetTickCount64();
}

void guiUpdate() {
    ULONGLONG now = GetTickCount64();
    if (lastPoll == 0 || now - lastPoll >= GUI_POLL_INTERVAL_MS) {
        guiPoll();
        lastPoll = now;
    }
    for (int i = 0; i < UI_CONTROL_GROUP_SIZE; i++) {
        controlGroups[i]->update();
    }
}

Client *guiClient(void) {
    return client;
}

int guiClientPort(void) {
    return clientPort;
}

const GuiSnapshot *guiGetSnapshot(void) {
    return &snapshot;
}

bool guiSnapshotCheat(CheatName cheat, bool *out) {
    if (!snapshot.cheatsValid || !out) return false;
    for (int i = 0; i < snapshot.cheatCount; i++) {
        CheatName cn;
        const char *name = clientCheatNameAt(i);
        if (name && clientCheatFromName(name, &cn) && cn == cheat) {
            *out = snapshot.cheats[i];
            return true;
        }
    }
    return false;
}

void guiSnapshotSetCheat(CheatName cheat, bool enabled) {
    if (!snapshot.cheatsValid) return;
    for (int i = 0; i < snapshot.cheatCount; i++) {
        CheatName cn;
        const char *name = clientCheatNameAt(i);
        if (name && clientCheatFromName(name, &cn) && cn == cheat) {
            snapshot.cheats[i] = enabled;
            return;
        }
    }
}

static void setupUi() {
    uiInitOptions o = {0};
    const char *err;
    err = uiInit(&o);
    if (err != NULL) {
        LOG_ERROR("Error initializing libui: %s", err);
        uiFreeInitError(err);
        exit(1);
    }
}

static void setupWindow() {
    window = uiNewWindow("Black Ops 1 Zombies Trainer", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    uiWindowSetIcon(window, IDI_ICON1);
    uiWindowOnClosing(window, onClosing, NULL);
    uiWindowSetMargined(window, 1);
    int screenWidth, screenHeight;
    uiScreenGetResolution(&screenWidth, &screenHeight);
    uiWindowSetPosition(window, screenWidth / 2 - (WINDOW_WIDTH/2), screenHeight / 2 - (WINDOW_HEIGHT/2));
    uiWindowSetResizeable(window, 0);
}

static uiControl* buildWindowContent() {
    // Control Groups
    UIControlGroup *playerControlGroup = uiPlayerBuildControlGroup();
    UIControlGroup *cheatsControlGroup = uiHacksBuildControlGroup();
    UIControlGroup *weaponsControlGroup = uiWeaponsBuildControlGroup();
    UIControlGroup *teleportControlGroup = uiTeleportBuildControlGroup();
    UIControlGroup *characterControlGroup = uiCharacterBuildControlGroup();
    UIControlGroup *graphicsControlGroup = uiGraphicsBuildControlGroup();
    UIControlGroup *gameControlGroup = uiGameBuildControlGroup();
    UIControlGroup *aboutControlGroup = uiAboutBuildControlGroup();

    // Save Control Groups for update
    controlGroups[0] = playerControlGroup;
    controlGroups[1] = cheatsControlGroup;
    controlGroups[2] = weaponsControlGroup;
    controlGroups[3] = teleportControlGroup;
    controlGroups[4] = characterControlGroup;
    controlGroups[5] = graphicsControlGroup;
    controlGroups[6] = gameControlGroup;
    controlGroups[7] = aboutControlGroup;

    // UI Groups
    uiControl *playerGroup = playerControlGroup->build(client, window);
    uiControl *cheatGroup = cheatsControlGroup->build(client, window);
    uiControl *weaponsGroup = weaponsControlGroup->build(client, window);
    uiControl *teleportGroup = teleportControlGroup->build(client, window);
    uiControl *characterGroup = characterControlGroup->build(client, window);
    uiControl *graphicsGroup = graphicsControlGroup->build(client, window);
    uiControl *gameGroup = gameControlGroup->build(client, window);
    uiControl *aboutGroup = aboutControlGroup->build(client, window);

    uiBox *ctVBox = uiNewVerticalBox();
    uiBoxSetPadded(ctVBox, 1);
    uiBoxAppend(ctVBox, characterGroup, 1);
    uiBoxAppend(ctVBox, teleportGroup, 1);

    uiBox *waVBox = uiNewVerticalBox();
    uiBoxSetPadded(waVBox, 1);
    uiBoxAppend(waVBox, weaponsGroup, 1);
    uiBoxAppend(waVBox, aboutGroup, 1);

    uiGrid *mainGrid = uiNewGrid();
    uiGridSetPadded(mainGrid, 1);

    uiGridAppend(mainGrid, playerGroup, 0, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(mainGrid, cheatGroup,  1, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(mainGrid, gameGroup,   2, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    uiGridAppend(mainGrid, graphicsGroup,       0, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(mainGrid, uiControl(ctVBox),   1, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(mainGrid, uiControl(waVBox),   2, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    return uiControl(mainGrid);
}

UIControlGroup *guiControlGroupCreate(uiControl *(*build)(Client *, uiWindow *), void (*update)()) {
    UIControlGroup *cg = (UIControlGroup*)malloc(sizeof(UIControlGroup));
    if (!cg) {
        LOG_ERROR("Couldn't allocate memory for UIControlGroup");
        return NULL;
    }
    cg->build = build;
    cg->update = update;
    return cg;
}

void guiInit(Client *clientInstance, int port) {
    client = clientInstance;
    clientPort = port;
    memset(&snapshot, 0, sizeof(snapshot));
    setupUi();
    setupWindow();
    uiControl *c = buildWindowContent();
    uiWindowSetChild(window,c);
}

void guiRun(void) {
    uiControlShow(uiControl(window));
    uiTimer(1000/60, onTimerUpdate, NULL); // ~60 FPS
    uiMain();
}

void guiCleanup(void) {
    uiUninit();
}

Color buildColor(uiColorButton *button) {
    double r, g, b, a;
    uiColorButtonColor(button, &r, &g, &b, &a);
    Color color = colorCreate(r*255, g*255, b*255, a*255);
    return color;
}

void setColorButton(uiColorButton *button, Color color) {
    uiColorButtonSetColor(button, color.r / 255.0, color.g / 255.0, color.b / 255.0, 1.0);
}
