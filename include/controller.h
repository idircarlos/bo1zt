#ifndef CONTROLLER_H_
#define CONTROLLER_H_

#include "win/process.h"
#include "logic/cheat.h"
#include "logic/state.h"
#include "logic/config.h"
#include "utils/list.h"

#define GAME_EXECUTABLE_NAME "BlackOps.exe"
#define GAME_WINDOW_NAME_PREFIX "Call of Duty"
#define DLL_NAME "bo1zt.dll"

typedef struct Controller Controller;

Controller* controllerCreate();
Process* controllerGetProcess(Controller *controller);
bool controllerLaunchGame(Controller *controller);
bool controllerCloseGame(Controller *controller);
bool controllerIsGameAttached(Controller *controller);
bool controllerAttachGame(Controller *controller);
bool controllerDetachGame(Controller *controller);
bool controllerIsGameRunning(Controller *controller);
bool controllerIsZombiesGameOngoing(Controller *controller);
bool controllerIsZombiesGamePaused(Controller *controller);
void controllerWaitUntilGameCloses(Controller *controller); // This method should be called from a different thread to not block the main thread (UI)
bool controllerIsGameWindowFocused(Controller *controller);
bool controllerIsGameWindowAttached(Controller *controller);
bool controllerIsGameReady(Controller *controller);
bool controllerIsChatOpen(Controller *controller);
bool controllerWriteToChatInput(Controller *controller, const char *text);
int controllerGetLevelElapsedTime(Controller *controller);
float controllerGetMovementSpeed(Controller *controller);
Level controllerGetLevelName(Controller *controller);
bool controllerTryAttachGameWindow(Controller *controller);
bool controllerGetCheat(Controller *controller, CheatName cheat);
bool controllerSetCheat(Controller *controller, CheatName cheat, bool enabled);
bool controllerSetSimpleCheat(Controller *controller, SimpleCheatName cheat, void *value);
bool controllerSetChatNameColor(Controller *controller, ChatColor color);
bool controllerIsCheatCheckboxChecked(Controller *controller, CheatName cheat);
int controllerUiGraphicsGetFpsCap(Controller *controller);
TeleportCoords *controllerGetPlayerCurrentCoords(Controller *controller);
bool controllerGiveAmmo(Controller *controller);
bool controllerGiveWeapons(Controller *controller, List *weapons);
bool controllerTakeWeapons(Controller *controller);
bool controllerSetRound(Controller *controller, int round);
State *controllerGetState(Controller *controller);
void controllerUpdateState(Controller *controller);
void controllerInitTrainerConfig(Controller *controller);
void controllerUpdateTrainerConfig(Controller *controller);
GameConfig controllerGetGameConfig(Controller *controller);
GraphicsConfig controllerGetGraphicsConfig(Controller *controller);
CustomizerConfig controllerGetCustomizerConfig(Controller *controller);
WidgetConfig controllerGetWidgetConfig(Controller *controller, int index);
BindsConfig controllerGetBindsConfig(Controller *controller);
Config *controllerGetConfig(Controller *controller);
void controllerResetConfig(Controller *controller, ConfigType type);
void controllerResetWidgetConfig(Controller *controller, int index);
void controllerResetBindsConfig(Controller *controller);
void controllerUpdateBindsConfig(Controller *controller, BindsConfig *bindsConfig);
void controllerDestroy(Controller *controller);

// Forward declaration for CheatManager
typedef struct CheatManager CheatManager;
CheatManager *controllerGetCheatManager(Controller *controller);

#endif // CONTROLLER_H_
