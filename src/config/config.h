#ifndef CONFIG_H_
#define CONFIG_H_

#include "../common/common.h"
#include <stdbool.h>

#define N_CONFIG_WIDGETS 3

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
} WidgetConfig;

typedef struct {
    GameConfig game;
    GraphicsConfig graphics;
    CustomizerConfig customizer;
    WidgetConfig widgets[N_CONFIG_WIDGETS];
} Config;

typedef enum {
    CONFIG_GAME = 0,
    CONFIG_GRAPHICS,
    CONFIG_CUSTOMIZER,
    CONFIG_WIDGETS,
} ConfigType;

Config* configCreate();
bool configSave(Config *config);
void configReset(Config *config);
void configResetGame(Config *config);
void configResetGraphics(Config *config);
void configResetCustomizer(Config *config);
void configResetWidget(Config *config, int index);
void configDestroy(Config *config);

#endif // CONFIG_H_
