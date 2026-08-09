#include "service/customizer.h"
#include "service/service_internal.h"
#include "logic/cheat.h"

ServiceResult serviceCustomizerGet(Service *service, CustomizerConfig *configOut) {
    if (!service || !configOut) return SERVICE_INVALID_PARAM;
    *configOut = controllerGetCustomizerConfig(service->controller);
    return SERVICE_OK;
}

ServiceResult serviceCustomizerReset(Service *service) {
    if (!service) return SERVICE_INVALID_PARAM;
    controllerResetConfig(service->controller, CONFIG_CUSTOMIZER);
    return SERVICE_OK;
}

static void applyColor(Controller *c, SimpleCheatName cheat, RGBAColor *value) {
    if (controllerIsGameAttached(c)) controllerSetSimpleCheat(c, cheat, value);
}

static void applyInt(Controller *c, SimpleCheatName cheat, int *value) {
    if (controllerIsGameAttached(c)) controllerSetSimpleCheat(c, cheat, value);
}

ServiceResult serviceCustomizerUpdate(Service *service, const CustomizerPatch *patch) {
    if (!service || !patch) return SERVICE_INVALID_PARAM;
    Controller *c = service->controller;
    Config *config = controllerGetConfig(c);
    if (!config) return SERVICE_ENGINE_FAILED;
    CustomizerConfig *cz = &config->customizer;

    if (patch->hasScoreBackground) {
        cz->scoreBackground = patch->scoreBackground;
        applyColor(c, SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_BACKGROUND, &cz->scoreBackground);
    }
    if (patch->hasScorePlayer1) {
        cz->scorePlayer1 = patch->scorePlayer1;
        applyColor(c, SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P1, &cz->scorePlayer1);
    }
    if (patch->hasScorePlayer2) {
        cz->scorePlayer2 = patch->scorePlayer2;
        applyColor(c, SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P2, &cz->scorePlayer2);
    }
    if (patch->hasScorePlayer3) {
        cz->scorePlayer3 = patch->scorePlayer3;
        applyColor(c, SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P3, &cz->scorePlayer3);
    }
    if (patch->hasScorePlayer4) {
        cz->scorePlayer4 = patch->scorePlayer4;
        applyColor(c, SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P4, &cz->scorePlayer4);
    }
    if (patch->hasReloadWarnPrimary) {
        cz->reloadWarnPrimary = patch->reloadWarnPrimary;
        applyColor(c, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_RELOAD_PRIMARY, &cz->reloadWarnPrimary);
    }
    if (patch->hasReloadWarnSecondary) {
        cz->reloadWarnSecondary = patch->reloadWarnSecondary;
        applyColor(c, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_RELOAD_SECONDARY, &cz->reloadWarnSecondary);
    }
    if (patch->hasLowAmmoWarnPrimary) {
        cz->lowAmmoWarnPrimary = patch->lowAmmoWarnPrimary;
        applyColor(c, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_LOW_AMMO_PRIMARY, &cz->lowAmmoWarnPrimary);
    }
    if (patch->hasLowAmmoWarnSecondary) {
        cz->lowAmmoWarnSecondary = patch->lowAmmoWarnSecondary;
        applyColor(c, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_LOW_AMMO_SECONDARY, &cz->lowAmmoWarnSecondary);
    }
    if (patch->hasNoAmmoWarnPrimary) {
        cz->noAmmoWarnPrimary = patch->noAmmoWarnPrimary;
        applyColor(c, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_NO_AMMO_PRIMARY, &cz->noAmmoWarnPrimary);
    }
    if (patch->hasNoAmmoWarnSecondary) {
        cz->noAmmoWarnSecondary = patch->noAmmoWarnSecondary;
        applyColor(c, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_NO_AMMO_SECONDARY, &cz->noAmmoWarnSecondary);
    }
    if (patch->hasScoreboardTransparency) {
        cz->scoreboardTransparency = patch->scoreboardTransparency;
        applyInt(c, SIMPLE_CHEAT_NAME_CUSTOMIZER_TRANSPARENCY_SCOREBOARD, &cz->scoreboardTransparency);
    }
    if (patch->hasPointsTransparency) {
        cz->pointsTransparency = patch->pointsTransparency;
        applyInt(c, SIMPLE_CHEAT_NAME_CUSTOMIZER_TRANSPARENCY_POINTS, &cz->pointsTransparency);
    }
    if (patch->hasWarningFrequency) {
        cz->warningTransitionsFrequency = patch->warningFrequency;
        applyInt(c, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_FREQUENCY, &cz->warningTransitionsFrequency);
    }
    if (patch->hasWarningMin) {
        cz->warningTransitionsMin = patch->warningMin;
        applyInt(c, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_MIN, &cz->warningTransitionsMin);
    }
    if (patch->hasWarningMax) {
        cz->warningTransitionsMax = patch->warningMax;
        applyInt(c, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_MAX, &cz->warningTransitionsMax);
    }

    configSave(config);
    return SERVICE_OK;
}
