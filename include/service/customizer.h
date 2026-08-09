#ifndef SERVICE_CUSTOMIZER_H_
#define SERVICE_CUSTOMIZER_H_

#include "service.h"
#include "logic/config.h"
#include "utils/color.h"

typedef struct {
    bool hasScoreBackground;      RGBAColor scoreBackground;
    bool hasScorePlayer1;         RGBAColor scorePlayer1;
    bool hasScorePlayer2;         RGBAColor scorePlayer2;
    bool hasScorePlayer3;         RGBAColor scorePlayer3;
    bool hasScorePlayer4;         RGBAColor scorePlayer4;
    bool hasReloadWarnPrimary;    RGBAColor reloadWarnPrimary;
    bool hasReloadWarnSecondary;  RGBAColor reloadWarnSecondary;
    bool hasLowAmmoWarnPrimary;   RGBAColor lowAmmoWarnPrimary;
    bool hasLowAmmoWarnSecondary; RGBAColor lowAmmoWarnSecondary;
    bool hasNoAmmoWarnPrimary;    RGBAColor noAmmoWarnPrimary;
    bool hasNoAmmoWarnSecondary;  RGBAColor noAmmoWarnSecondary;
    bool hasScoreboardTransparency; int scoreboardTransparency;
    bool hasPointsTransparency;     int pointsTransparency;
    bool hasWarningFrequency;       int warningFrequency;
    bool hasWarningMin;             int warningMin;
    bool hasWarningMax;             int warningMax;
} CustomizerPatch;

ServiceResult serviceCustomizerGet(Service *service, CustomizerConfig *configOut);
ServiceResult serviceCustomizerReset(Service *service);
ServiceResult serviceCustomizerUpdate(Service *service, const CustomizerPatch *patch);

#endif // SERVICE_CUSTOMIZER_H_
