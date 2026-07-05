#ifndef GUI_H_
#define GUI_H_

#include <ui.h>
#include "client.h"
#include "client/state.h"    // GameState
#include "client/game.h"     // GameStatus, GameConfigInfo
#include "logic/cheat.h"     // CheatName
#include "logic/config.h"    // GraphicsConfig

typedef struct {
    bool valid;
    GameStatus status;          bool statusValid;
    GameState state;            bool stateValid;
    GraphicsConfig graphics;    bool graphicsValid;
    GameConfigInfo gameConfig;  bool gameConfigValid;
    bool cheats[32];
    int cheatCount;
    bool cheatsValid;
} GuiSnapshot;

typedef struct {
    uiControl *(*build)(Client *, uiWindow *);
    void (*update)();
} UIControlGroup;

UIControlGroup *guiControlGroupCreate(uiControl *(*build)(Client *, uiWindow *), void (*update)());
void guiInit(Client *client, int clientPort);
void guiRun(void);
void guiUpdate();
void guiCleanup(void);

Client *guiClient(void);
int guiClientPort(void);
const GuiSnapshot *guiGetSnapshot(void);
void guiPollNow(void);
bool guiSnapshotCheat(CheatName cheat, bool *out);
void guiSnapshotSetCheat(CheatName cheat, bool enabled);

#endif // GUI_H_
