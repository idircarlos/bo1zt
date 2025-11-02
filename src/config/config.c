#include "config.h"
#include "../logger/logger.h"
#include <iniparser.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

#define INI_FILE_NAME "BO1ZT.ini"
#define STRFMT_BUFF_SIZE 1024
#define COLOR_INI_FMT "Color(%d,%d,%d)"
#define COLOR_INI_DEFAULT "Color(255,255,255)"

// Aux
static inline char *strfmt(char *buff, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(buff, STRFMT_BUFF_SIZE, fmt, args);
    va_end(args);
    return buff;
}

static Color colorFromString(const char *colorString) {
    uint8_t r = 255, g = 255, b = 255;
    if (sscanf(colorString, "Color(%hhu,%hhu,%hhu)", &r, &g, &b) != 3) {
        LOG_ERROR("Invalid color string: %s\n", colorString);
    }
    return colorCreate(r, g, b);
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

    config->fov = iniparser_getint(dictionary, "Graphics:FOV", config->fov);
    config->fovScale = iniparser_getint(dictionary, "Graphics:FOVScale", config->fovScale);
    config->fpsCap = iniparser_getint(dictionary, "Graphics:FPSCap", config->fpsCap);
    config->borderless = iniparser_getboolean(dictionary, "Graphics:Borderless", config->borderless);
    config->unlimitFps = iniparser_getboolean(dictionary, "Graphics:UnlimitFps", config->unlimitFps);
    config->disableHud = iniparser_getboolean(dictionary, "Graphics:DisableHud", config->disableHud);
    config->disableFog = iniparser_getboolean(dictionary, "Graphics:DisableFog", config->disableFog);
    config->fullbright = iniparser_getboolean(dictionary, "Graphics:Fullbright", config->fullbright);
    config->colorized = iniparser_getboolean(dictionary, "Graphics:Colorized", config->colorized);
    config->scoreBackground = colorFromString(iniparser_getstring(dictionary, "Customizer:ScoreBackground", COLOR_INI_DEFAULT));
    config->scorePlayer1 = colorFromString(iniparser_getstring(dictionary, "Customizer:ScorePlayer1", COLOR_INI_DEFAULT));
    config->scorePlayer2 = colorFromString(iniparser_getstring(dictionary, "Customizer:ScorePlayer2", COLOR_INI_DEFAULT));
    config->scorePlayer3 = colorFromString(iniparser_getstring(dictionary, "Customizer:ScorePlayer3", COLOR_INI_DEFAULT));
    config->scorePlayer4 = colorFromString(iniparser_getstring(dictionary, "Customizer:ScorePlayer4", COLOR_INI_DEFAULT));
    config->reloadWarnPrimary = colorFromString(iniparser_getstring(dictionary, "Customizer:ReloadWarnPrimary", COLOR_INI_DEFAULT));
    config->reloadWarnSecondary = colorFromString(iniparser_getstring(dictionary, "Customizer:ReloadWarnSecondary", COLOR_INI_DEFAULT));
    config->lowAmmoWarnPrimary = colorFromString(iniparser_getstring(dictionary, "Customizer:LowAmmoWarnPrimary", COLOR_INI_DEFAULT));
    config->lowAmmoWarnSecondary = colorFromString(iniparser_getstring(dictionary, "Customizer:LowAmmoWarnSecondary", COLOR_INI_DEFAULT));
    config->noAmmoWarnPrimary = colorFromString(iniparser_getstring(dictionary, "Customizer:NoAmmoWarnPrimary", COLOR_INI_DEFAULT));
    config->noAmmoWarnSecondary = colorFromString(iniparser_getstring(dictionary, "Customizer:NoAmmoWarnSecondary", COLOR_INI_DEFAULT));
    config->scoreboardTransparency = iniparser_getint(dictionary, "Customizer:ScoreboardTransparency", config->scoreboardTransparency);
    config->pointsTransparency = iniparser_getint(dictionary, "Customizer:PointsTransparency", config->pointsTransparency);
    config->warningTransitionsFrequency = iniparser_getint(dictionary, "Customizer:WarningTransitionsFrequency", config->warningTransitionsFrequency);
    config->warningTransitionsMin = iniparser_getint(dictionary, "Customizer:WarningTransitionsMin", config->warningTransitionsMin);
    config->warningTransitionsMax = iniparser_getint(dictionary, "Customizer:WarningTransitionsMax", config->warningTransitionsMax);

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
    ret += iniparser_set(dictionary, "Graphics", NULL);
    ret += iniparser_set(dictionary, "Graphics:FOV", strfmt(valueBuffer, "%d", config->fov));
    ret += iniparser_set(dictionary, "Graphics:FOVScale", strfmt(valueBuffer, "%d", config->fovScale));
    ret += iniparser_set(dictionary, "Graphics:FPSCap", strfmt(valueBuffer, "%d", config->fpsCap));
    ret += iniparser_set(dictionary, "Graphics:Borderless", strfmt(valueBuffer, "%d", config->borderless));
    ret += iniparser_set(dictionary, "Graphics:UnlimitFps", strfmt(valueBuffer, "%d", config->unlimitFps));
    ret += iniparser_set(dictionary, "Graphics:DisableHud", strfmt(valueBuffer, "%d", config->disableHud));
    ret += iniparser_set(dictionary, "Graphics:DisableFog", strfmt(valueBuffer, "%d", config->disableFog));
    ret += iniparser_set(dictionary, "Graphics:Fullbright", strfmt(valueBuffer, "%d", config->fullbright));
    ret += iniparser_set(dictionary, "Graphics:Colorized", strfmt(valueBuffer, "%d", config->colorized));
    ret += iniparser_set(dictionary, "Customizer", NULL);
    ret += iniparser_set(dictionary, "Customizer:ScoreBackground", strfmt(valueBuffer, COLOR_INI_FMT, config->scoreBackground.r, config->scoreBackground.g, config->scoreBackground.b));
    ret += iniparser_set(dictionary, "Customizer:ScorePlayer1", strfmt(valueBuffer, COLOR_INI_FMT, config->scorePlayer1.r, config->scorePlayer1.g, config->scorePlayer1.b));
    ret += iniparser_set(dictionary, "Customizer:ScorePlayer2", strfmt(valueBuffer, COLOR_INI_FMT, config->scorePlayer2.r, config->scorePlayer2.g, config->scorePlayer2.b));
    ret += iniparser_set(dictionary, "Customizer:ScorePlayer3", strfmt(valueBuffer, COLOR_INI_FMT, config->scorePlayer3.r, config->scorePlayer3.g, config->scorePlayer3.b));
    ret += iniparser_set(dictionary, "Customizer:ScorePlayer4", strfmt(valueBuffer, COLOR_INI_FMT, config->scorePlayer4.r, config->scorePlayer4.g, config->scorePlayer4.b));
    ret += iniparser_set(dictionary, "Customizer:ReloadWarnPrimary", strfmt(valueBuffer, COLOR_INI_FMT, config->reloadWarnPrimary.r, config->reloadWarnPrimary.g, config->reloadWarnPrimary.b));
    ret += iniparser_set(dictionary, "Customizer:ReloadWarnSecondary", strfmt(valueBuffer, COLOR_INI_FMT, config->reloadWarnSecondary.r, config->reloadWarnSecondary.g, config->reloadWarnSecondary.b));
    ret += iniparser_set(dictionary, "Customizer:LowAmmoWarnPrimary", strfmt(valueBuffer, COLOR_INI_FMT, config->lowAmmoWarnPrimary.r, config->lowAmmoWarnPrimary.g, config->lowAmmoWarnPrimary.b));
    ret += iniparser_set(dictionary, "Customizer:LowAmmoWarnSecondary", strfmt(valueBuffer, COLOR_INI_FMT, config->lowAmmoWarnSecondary.r, config->lowAmmoWarnSecondary.g, config->lowAmmoWarnSecondary.b));
    ret += iniparser_set(dictionary, "Customizer:NoAmmoWarnPrimary", strfmt(valueBuffer, COLOR_INI_FMT, config->noAmmoWarnPrimary.r, config->noAmmoWarnPrimary.g, config->noAmmoWarnPrimary.b));
    ret += iniparser_set(dictionary, "Customizer:NoAmmoWarnSecondary", strfmt(valueBuffer, COLOR_INI_FMT, config->noAmmoWarnSecondary.r, config->noAmmoWarnSecondary.g, config->noAmmoWarnSecondary.b));
    ret += iniparser_set(dictionary, "Customizer:ScoreboardTransparency", strfmt(valueBuffer, "%d", config->scoreboardTransparency));
    ret += iniparser_set(dictionary, "Customizer:PointsTransparency", strfmt(valueBuffer, "%d", config->pointsTransparency));
    ret += iniparser_set(dictionary, "Customizer:WarningTransitionsFrequency", strfmt(valueBuffer, "%d", config->warningTransitionsFrequency));
    ret += iniparser_set(dictionary, "Customizer:WarningTransitionsMin", strfmt(valueBuffer, "%d", config->warningTransitionsMin));
    ret += iniparser_set(dictionary, "Customizer:WarningTransitionsMax", strfmt(valueBuffer, "%d", config->warningTransitionsMax));
    if (ret < 0) {
        LOG_ERROR("Error setting ini values\n");
        return false;
    }
    iniparser_dump_ini(dictionary, ini);
    fclose(ini);
    return false;
}

void configReset(Config *config) {
    config->fov = 90;
    config->fovScale = 100;
    config->fpsCap = 60;
    config->borderless = false;
    config->unlimitFps = false;
    config->disableHud = false;
    config->disableFog = false;
    config->fullbright = false;
    config->colorized = false;
    config->scoreBackground = colorCreate(255, 255, 255);
    config->scorePlayer1 = colorCreate(255, 255, 255);
    config->scorePlayer2 = colorCreate(255, 255, 255);
    config->scorePlayer3 = colorCreate(255, 255, 255);
    config->scorePlayer4 = colorCreate(255, 255, 255);
    config->reloadWarnPrimary = colorCreate(255, 0, 0);
    config->reloadWarnSecondary = colorCreate(255, 0, 0);
    config->lowAmmoWarnPrimary = colorCreate(255, 165, 0);
    config->lowAmmoWarnSecondary = colorCreate(55, 165, 0);
    config->noAmmoWarnPrimary = colorCreate(255, 0, 0);
    config->noAmmoWarnSecondary = colorCreate(255, 0, 0);
    config->scoreboardTransparency = 100;
    config->pointsTransparency = 100;
    config->warningTransitionsFrequency = 5;
    config->warningTransitionsMin = 50;
    config->warningTransitionsMax = 100;
}

void configDestroy(Config *config) {
    if (config) free(config);
}
