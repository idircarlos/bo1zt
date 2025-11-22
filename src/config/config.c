#include "config.h"
#include "../logger/logger.h"
#include "../gui/game/widgets/widgets.h"
#include <iniparser.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#define INI_FILE_NAME "BO1ZT.ini"
#define STRFMT_BUFF_SIZE 1024
#define COLOR_INI_FMT "Color(%hhu,%hhu,%hhu,%hhu)"
#define COLOR_INI_DEFAULT "Color(111,111,111,111)"
#define RECT_INI_FMT "Rect(%u,%u,%u,%u)"
#define RECT_INI_DEFAULT "Rect(111,111,111,111)"

// Aux
static inline char *strfmt(char *buff, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(buff, STRFMT_BUFF_SIZE, fmt, args);
    va_end(args);
    return buff;
}

static Color colorFromString(const char *colorString) {
    uint8_t r = 255, g = 255, b = 255, a = 255;
    if (sscanf(colorString, "Color(%hhu,%hhu,%hhu,%hhu)", &r, &g, &b, &a) != 4) {
        LOG_ERROR("Invalid color string: %s\n", colorString);
    }
    return colorCreate(r, g, b, a);
}

static Rect rectFromString(const char *rectString) {
    uint32_t x = 0, y = 0, w = 0, h = 0;
    if (sscanf(rectString, "Rect(%u,%u,%u,%u)", &x, &y, &w, &h) != 4) {
        LOG_ERROR("Invalid rect string: %s\n", rectString);
    }
    return rectCreate(x, y, w, h);
}

static bool iniFileExists() {
    FILE *ini = fopen(INI_FILE_NAME, "r");
    if (!ini) return false;
    fclose(ini);
    return true;
}

static bool configLoad(Config *config) {
    dictionary *dictionary = iniparser_load(INI_FILE_NAME);
    if (!dictionary) {
        fprintf(stderr, "cannot parse file: %s\n", INI_FILE_NAME);
        return false;
    }

    config->graphics.fov = iniparser_getint(dictionary, "Game:FixMovementSpeed", config->game.fixMovementSpeed);
    config->graphics.fov = iniparser_getint(dictionary, "Game:ShowFPS", config->game.showFps);
    strcpy(config->game.hostname, iniparser_getstring(dictionary, "Game:Hostname", config->game.hostname));
    strcpy(config->game.location, iniparser_getstring(dictionary, "Game:Location", config->game.location));
    config->graphics.fov = iniparser_getint(dictionary, "Graphics:FOV", config->graphics.fov);
    config->graphics.fovScale = iniparser_getint(dictionary, "Graphics:FOVScale", config->graphics.fovScale);
    config->graphics.fpsCap = iniparser_getint(dictionary, "Graphics:FPSCap", config->graphics.fpsCap);
    config->graphics.borderless = iniparser_getboolean(dictionary, "Graphics:Borderless", config->graphics.borderless);
    config->graphics.unlimitFps = iniparser_getboolean(dictionary, "Graphics:UnlimitFps", config->graphics.unlimitFps);
    config->graphics.disableHud = iniparser_getboolean(dictionary, "Graphics:DisableHud", config->graphics.disableHud);
    config->graphics.disableFog = iniparser_getboolean(dictionary, "Graphics:DisableFog", config->graphics.disableFog);
    config->graphics.fullbright = iniparser_getboolean(dictionary, "Graphics:Fullbright", config->graphics.fullbright);
    config->graphics.colorized = iniparser_getboolean(dictionary, "Graphics:Colorized", config->graphics.colorized);
    config->customizer.scoreBackground = colorFromString(iniparser_getstring(dictionary, "Customizer:ScoreBackground", COLOR_INI_DEFAULT));
    config->customizer.scorePlayer1 = colorFromString(iniparser_getstring(dictionary, "Customizer:ScorePlayer1", COLOR_INI_DEFAULT));
    config->customizer.scorePlayer2 = colorFromString(iniparser_getstring(dictionary, "Customizer:ScorePlayer2", COLOR_INI_DEFAULT));
    config->customizer.scorePlayer3 = colorFromString(iniparser_getstring(dictionary, "Customizer:ScorePlayer3", COLOR_INI_DEFAULT));
    config->customizer.scorePlayer4 = colorFromString(iniparser_getstring(dictionary, "Customizer:ScorePlayer4", COLOR_INI_DEFAULT));
    config->customizer.reloadWarnPrimary = colorFromString(iniparser_getstring(dictionary, "Customizer:ReloadWarnPrimary", COLOR_INI_DEFAULT));
    config->customizer.reloadWarnSecondary = colorFromString(iniparser_getstring(dictionary, "Customizer:ReloadWarnSecondary", COLOR_INI_DEFAULT));
    config->customizer.lowAmmoWarnPrimary = colorFromString(iniparser_getstring(dictionary, "Customizer:LowAmmoWarnPrimary", COLOR_INI_DEFAULT));
    config->customizer.lowAmmoWarnSecondary = colorFromString(iniparser_getstring(dictionary, "Customizer:LowAmmoWarnSecondary", COLOR_INI_DEFAULT));
    config->customizer.noAmmoWarnPrimary = colorFromString(iniparser_getstring(dictionary, "Customizer:NoAmmoWarnPrimary", COLOR_INI_DEFAULT));
    config->customizer.noAmmoWarnSecondary = colorFromString(iniparser_getstring(dictionary, "Customizer:NoAmmoWarnSecondary", COLOR_INI_DEFAULT));
    config->customizer.scoreboardTransparency = iniparser_getint(dictionary, "Customizer:ScoreboardTransparency", config->customizer.scoreboardTransparency);
    config->customizer.pointsTransparency = iniparser_getint(dictionary, "Customizer:PointsTransparency", config->customizer.pointsTransparency);
    config->customizer.warningTransitionsFrequency = iniparser_getint(dictionary, "Customizer:WarningTransitionsFrequency", config->customizer.warningTransitionsFrequency);
    config->customizer.warningTransitionsMin = iniparser_getint(dictionary, "Customizer:WarningTransitionsMin", config->customizer.warningTransitionsMin);
    config->customizer.warningTransitionsMax = iniparser_getint(dictionary, "Customizer:WarningTransitionsMax", config->customizer.warningTransitionsMax);
    for (int i = 0; i < N_CONFIG_WIDGETS; i++) {
        char keyEnabled[128];
        char keyFont[128];
        char keyTextColor[128];
        char keyHideOnDefault[128];
        char keyRect[128];
        char keyFontSize[128];
        snprintf(keyEnabled, sizeof(keyEnabled), "Widgets:%sEnabled", uiWidgetsGetName(i));
        snprintf(keyFont, sizeof(keyFont), "Widgets:%sFont", uiWidgetsGetName(i));
        snprintf(keyTextColor, sizeof(keyTextColor), "Widgets:%sTextColor", uiWidgetsGetName(i));
        snprintf(keyHideOnDefault, sizeof(keyHideOnDefault), "Widgets:%sHideOnDefault", uiWidgetsGetName(i));
        snprintf(keyRect, sizeof(keyRect), "Widgets:%sRect", uiWidgetsGetName(i));
        snprintf(keyFontSize, sizeof(keyFontSize), "Widgets:%sFontSize", uiWidgetsGetName(i));
        config->widgets[i].enabled = iniparser_getboolean(dictionary, keyEnabled, config->widgets[i].enabled);
        strcpy(config->widgets[i].font, iniparser_getstring(dictionary, keyFont, config->widgets[i].font));
        config->widgets[i].textColor = colorFromString(iniparser_getstring(dictionary, keyTextColor, COLOR_INI_DEFAULT));
        config->widgets[i].hideOnDefault = iniparser_getboolean(dictionary, keyHideOnDefault, config->widgets[i].hideOnDefault);
        config->widgets[i].rect = rectFromString(iniparser_getstring(dictionary, keyRect, RECT_INI_DEFAULT));
        config->widgets[i].fontSize = iniparser_getint(dictionary, keyFontSize, config->widgets[i].fontSize);
    }

    iniparser_freedict(dictionary);
    return true;
}

Config* configCreate() {
    Config *config = (Config*)malloc(sizeof(Config));
    if (!config) return NULL;

    if (iniFileExists()) {
        configLoad(config);
        return config;
    }

    configReset(config);
    for (int i = 0; i < N_CONFIG_WIDGETS; i++) {
        config->widgets[i].rect = uiWidgetsGetDefaultRect(i);
        config->widgets[i].fontSize = uiWidgetsGetDefaultFontSize(i);
    }
    configSave(config);
    return config;
}

bool configSave(Config *config) {
    FILE *ini = fopen(INI_FILE_NAME, "w+");
    if (!ini) {
        fprintf(stderr, "Cannot create trainer.ini\n");
        return false;
    }

    int ret = 0; 
    dictionary *dictionary = iniparser_load(INI_FILE_NAME);

    if (!dictionary) {
        fprintf(stderr, "cannot parse file: %s\n", INI_FILE_NAME);
        return -1;
    }

    char valueBuffer[1024] = "";
    ret += iniparser_set(dictionary, "Game", NULL);
    ret += iniparser_set(dictionary, "Game:FixMovementSpeed", strfmt(valueBuffer, "%d", config->game.fixMovementSpeed));
    ret += iniparser_set(dictionary, "Game:ShowFPS", strfmt(valueBuffer, "%d", config->game.showFps));
    ret += iniparser_set(dictionary, "Game:Hostname", strfmt(valueBuffer, "%s", config->game.hostname));
    ret += iniparser_set(dictionary, "Game:Location", strfmt(valueBuffer, "%s", config->game.location));
    ret += iniparser_set(dictionary, "Graphics", NULL);
    ret += iniparser_set(dictionary, "Graphics:FOV", strfmt(valueBuffer, "%d", config->graphics.fov));
    ret += iniparser_set(dictionary, "Graphics:FOVScale", strfmt(valueBuffer, "%d", config->graphics.fovScale));
    ret += iniparser_set(dictionary, "Graphics:FPSCap", strfmt(valueBuffer, "%d", config->graphics.fpsCap));
    ret += iniparser_set(dictionary, "Graphics:Borderless", strfmt(valueBuffer, "%d", config->graphics.borderless));
    ret += iniparser_set(dictionary, "Graphics:UnlimitFps", strfmt(valueBuffer, "%d", config->graphics.unlimitFps));
    ret += iniparser_set(dictionary, "Graphics:DisableHud", strfmt(valueBuffer, "%d", config->graphics.disableHud));
    ret += iniparser_set(dictionary, "Graphics:DisableFog", strfmt(valueBuffer, "%d", config->graphics.disableFog));
    ret += iniparser_set(dictionary, "Graphics:Fullbright", strfmt(valueBuffer, "%d", config->graphics.fullbright));
    ret += iniparser_set(dictionary, "Graphics:Colorized", strfmt(valueBuffer, "%d", config->graphics.colorized));
    ret += iniparser_set(dictionary, "Customizer", NULL);
    ret += iniparser_set(dictionary, "Customizer:ScoreBackground", strfmt(valueBuffer, COLOR_INI_FMT, config->customizer.scoreBackground.r, config->customizer.scoreBackground.g, config->customizer.scoreBackground.b, config->customizer.scoreBackground.a));
    ret += iniparser_set(dictionary, "Customizer:ScorePlayer1", strfmt(valueBuffer, COLOR_INI_FMT, config->customizer.scorePlayer1.r, config->customizer.scorePlayer1.g, config->customizer.scorePlayer1.b, config->customizer.scorePlayer1.a));
    ret += iniparser_set(dictionary, "Customizer:ScorePlayer2", strfmt(valueBuffer, COLOR_INI_FMT, config->customizer.scorePlayer2.r, config->customizer.scorePlayer2.g, config->customizer.scorePlayer2.b, config->customizer.scorePlayer2.a));
    ret += iniparser_set(dictionary, "Customizer:ScorePlayer3", strfmt(valueBuffer, COLOR_INI_FMT, config->customizer.scorePlayer3.r, config->customizer.scorePlayer3.g, config->customizer.scorePlayer3.b, config->customizer.scorePlayer3.a));
    ret += iniparser_set(dictionary, "Customizer:ScorePlayer4", strfmt(valueBuffer, COLOR_INI_FMT, config->customizer.scorePlayer4.r, config->customizer.scorePlayer4.g, config->customizer.scorePlayer4.b, config->customizer.scorePlayer4.a));
    ret += iniparser_set(dictionary, "Customizer:ReloadWarnPrimary", strfmt(valueBuffer, COLOR_INI_FMT, config->customizer.reloadWarnPrimary.r, config->customizer.reloadWarnPrimary.g, config->customizer.reloadWarnPrimary.b, config->customizer.reloadWarnPrimary.a));
    ret += iniparser_set(dictionary, "Customizer:ReloadWarnSecondary", strfmt(valueBuffer, COLOR_INI_FMT, config->customizer.reloadWarnSecondary.r, config->customizer.reloadWarnSecondary.g, config->customizer.reloadWarnSecondary.b, config->customizer.reloadWarnSecondary.a));
    ret += iniparser_set(dictionary, "Customizer:LowAmmoWarnPrimary", strfmt(valueBuffer, COLOR_INI_FMT, config->customizer.lowAmmoWarnPrimary.r, config->customizer.lowAmmoWarnPrimary.g, config->customizer.lowAmmoWarnPrimary.b, config->customizer.lowAmmoWarnPrimary.a));
    ret += iniparser_set(dictionary, "Customizer:LowAmmoWarnSecondary", strfmt(valueBuffer, COLOR_INI_FMT, config->customizer.lowAmmoWarnSecondary.r, config->customizer.lowAmmoWarnSecondary.g, config->customizer.lowAmmoWarnSecondary.b, config->customizer.lowAmmoWarnSecondary.a));
    ret += iniparser_set(dictionary, "Customizer:NoAmmoWarnPrimary", strfmt(valueBuffer, COLOR_INI_FMT, config->customizer.noAmmoWarnPrimary.r, config->customizer.noAmmoWarnPrimary.g, config->customizer.noAmmoWarnPrimary.b, config->customizer.noAmmoWarnPrimary.a));
    ret += iniparser_set(dictionary, "Customizer:NoAmmoWarnSecondary", strfmt(valueBuffer, COLOR_INI_FMT, config->customizer.noAmmoWarnSecondary.r, config->customizer.noAmmoWarnSecondary.g, config->customizer.noAmmoWarnSecondary.b, config->customizer.noAmmoWarnSecondary.a));
    ret += iniparser_set(dictionary, "Customizer:ScoreboardTransparency", strfmt(valueBuffer, "%d", config->customizer.scoreboardTransparency));
    ret += iniparser_set(dictionary, "Customizer:PointsTransparency", strfmt(valueBuffer, "%d", config->customizer.pointsTransparency));
    ret += iniparser_set(dictionary, "Customizer:WarningTransitionsFrequency", strfmt(valueBuffer, "%d", config->customizer.warningTransitionsFrequency));
    ret += iniparser_set(dictionary, "Customizer:WarningTransitionsMin", strfmt(valueBuffer, "%d", config->customizer.warningTransitionsMin));
    ret += iniparser_set(dictionary, "Customizer:WarningTransitionsMax", strfmt(valueBuffer, "%d", config->customizer.warningTransitionsMax));
    ret += iniparser_set(dictionary, "Widgets", NULL);
    for (int i = 0; i < N_CONFIG_WIDGETS; i++) {
        char keyEnabled[128];
        char keyFont[128];
        char keyTextColor[128];
        char keyHideOnDefault[128];
        char keyRect[128];
        char keyFontSize[128];
        snprintf(keyEnabled, sizeof(keyEnabled), "Widgets:%sEnabled", uiWidgetsGetName(i));
        snprintf(keyFont, sizeof(keyFont), "Widgets:%sFont", uiWidgetsGetName(i));
        snprintf(keyTextColor, sizeof(keyTextColor), "Widgets:%sTextColor", uiWidgetsGetName(i));
        snprintf(keyHideOnDefault, sizeof(keyHideOnDefault), "Widgets:%sHideOnDefault", uiWidgetsGetName(i));
        snprintf(keyRect, sizeof(keyRect), "Widgets:%sRect", uiWidgetsGetName(i));
        snprintf(keyFontSize, sizeof(keyFontSize), "Widgets:%sFontSize", uiWidgetsGetName(i));
        ret += iniparser_set(dictionary, keyEnabled, strfmt(valueBuffer, "%d", config->widgets[i].enabled));
        ret += iniparser_set(dictionary, keyFont, strfmt(valueBuffer, "%s", config->widgets[i].font));
        ret += iniparser_set(dictionary, keyTextColor, strfmt(valueBuffer, COLOR_INI_FMT, config->widgets[i].textColor.r, config->widgets[i].textColor.g, config->widgets[i].textColor.b, config->widgets[i].textColor.a));
        ret += iniparser_set(dictionary, keyHideOnDefault, strfmt(valueBuffer, "%d", config->widgets[i].hideOnDefault));
        ret += iniparser_set(dictionary, keyRect, strfmt(valueBuffer, RECT_INI_FMT, config->widgets[i].rect.x, config->widgets[i].rect.y, config->widgets[i].rect.w, config->widgets[i].rect.h));
        ret += iniparser_set(dictionary, keyFontSize, strfmt(valueBuffer, "%d", config->widgets[i].fontSize));
    }

    if (ret < 0) {
        LOG_ERROR("Error setting ini values\n");
        return false;
    }
    iniparser_dump_ini(dictionary, ini);
    dictionary_del(dictionary);
    fclose(ini);
    return true;
}

void configReset(Config *config) {
    configResetGame(config);
    configResetGraphics(config);
    configResetCustomizer(config);
    for (int i = 0; i < N_CONFIG_WIDGETS; i++) {
        configResetWidget(config, i);
    }
}

void configResetGame(Config *config) {
    GameConfig game;
    game.fixMovementSpeed = false;
    game.showFps = false;
    strcpy(game.hostname, "");
    strcpy(game.location, "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Call of Duty Black Ops\\BlackOps.exe");

    config->game = game;
}

void configResetGraphics(Config *config) {
    GraphicsConfig graphics = {
        .fov = 90,
        .fovScale = 100,
        .fpsCap = 185,
        .borderless = false,
        .unlimitFps = false,
        .disableHud = false,
        .disableFog = false,
        .fullbright = false,
        .colorized = false,
    };

    config->graphics = graphics;
}

void configResetCustomizer(Config *config) {
    CustomizerConfig customizer = {
        .scoreBackground = colorCreate(255, 255, 255, 255),
        .scorePlayer1 = colorCreate(255, 255, 255, 255),
        .scorePlayer2 = colorCreate(255, 255, 255, 255),
        .scorePlayer3 = colorCreate(255, 255, 255, 255),
        .scorePlayer4 = colorCreate(255, 255, 255, 255),
        .reloadWarnPrimary = colorCreate(255, 0, 0, 255),
        .reloadWarnSecondary = colorCreate(255, 0, 0, 255),
        .lowAmmoWarnPrimary = colorCreate(255, 165, 0, 255),
        .lowAmmoWarnSecondary = colorCreate(55, 165, 0, 255),
        .noAmmoWarnPrimary = colorCreate(255, 0, 0, 255),
        .noAmmoWarnSecondary = colorCreate(255, 0, 0, 255),
        .scoreboardTransparency = 100,
        .pointsTransparency = 100,
        .warningTransitionsFrequency = 5,
        .warningTransitionsMin = 50,
        .warningTransitionsMax = 100,
    };

    config->customizer = customizer;
}

void configResetWidget(Config *config, int index) {
    WidgetConfig widget;
    widget.enabled = false;
    strcpy(widget.font, "Digital-7 Mono");
    widget.textColor = colorCreate(255, 255, 255, 255);
    widget.hideOnDefault = false;    
    config->widgets[index] = widget;
}

void configDestroy(Config *config) {
    if (config) free(config);
}
