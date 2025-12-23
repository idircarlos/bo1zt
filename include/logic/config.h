#ifndef CONFIG_H_
#define CONFIG_H_

#include "utils/common.h"
#include <stdbool.h>

#define N_CONFIG_WIDGETS 4
#define MAX_BINDS 100
#define MAX_KEY_NAME_LENGTH 32
#define MAX_COMMAND_LENGTH 256

typedef struct {
    bool fixMovementSpeed;
    bool showFps;
    char hostname[256];
    char location[256];
} GameConfig;

typedef struct {
    int fov;
    int fovScale;
    int fpsCap;
    bool borderless;
    bool unlimitFps;
    bool disableHud;
    bool disableFog;
    bool fullbright;
    bool colorized;
} GraphicsConfig;

typedef struct {
    Color scoreBackground;
    Color scorePlayer1;
    Color scorePlayer2;
    Color scorePlayer3;
    Color scorePlayer4;
    Color reloadWarnPrimary;
    Color reloadWarnSecondary;
    Color lowAmmoWarnPrimary;
    Color lowAmmoWarnSecondary;
    Color noAmmoWarnPrimary;
    Color noAmmoWarnSecondary;
    int scoreboardTransparency;
    int pointsTransparency;
    int warningTransitionsFrequency;
    int warningTransitionsMin;
    int warningTransitionsMax;
} CustomizerConfig;

typedef struct {
    bool enabled;
    char font[256];
    Color textColor;
    bool hideOnDefault;
    // Non-reseteable props
    Rect rect;
    int fontSize;
} WidgetConfig;

typedef struct {
    bool godMode;
    bool noClip;
    bool invisible;
    bool infiniteAmmo;
    bool instantKill;
    bool noRecoil;
    bool smallCrosshair;
    bool fastGameplay;
    bool noShellshock;
    bool increaseKnifeRange;
    bool boxNeverMoves;
    bool thirdPerson;
} HacksConfig;

typedef struct {
    char keyName[MAX_KEY_NAME_LENGTH];
    char command[MAX_COMMAND_LENGTH];
} KeyBindConfig;

typedef struct {
    KeyBindConfig binds[MAX_BINDS];
    int bindCount;
} BindsConfig;

typedef struct {
    GameConfig game;
    GraphicsConfig graphics;
    CustomizerConfig customizer;
    HacksConfig hacks;
    WidgetConfig widgets[N_CONFIG_WIDGETS];
    BindsConfig binds;
} Config;

typedef enum {
    CONFIG_GAME = 0,
    CONFIG_GRAPHICS,
    CONFIG_CUSTOMIZER,
    CONFIG_WIDGETS,
    CONFIG_BINDS,
} ConfigType;

Config* configCreate();
bool configSave(Config *config);
void configReset(Config *config);
void configResetGame(Config *config);
void configResetGraphics(Config *config);
void configResetCustomizer(Config *config);
void configResetWidget(Config *config, int index);
void configResetBinds(Config *config);
void configResetHacks(Config *config);
void configDestroy(Config *config);

#endif // CONFIG_H_
