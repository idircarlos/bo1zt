#ifndef CONFIG_H_
#define CONFIG_H_

#include "../common/common.h"
#include <stdbool.h>

typedef struct {
    // Graphics
    int fov;
    int fovScale;
    int fpsCap;
    bool borderless;
    bool unlimitFps;
    bool disableHud;
    bool disableFog;
    bool fullbright;
    bool colorized;
    // Customizer
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
} Config;

Config* configCreate();
bool configSave(Config *config);
void configReset(Config *config);
void configDestroy(Config *config);

#endif // CONFIG_H_
