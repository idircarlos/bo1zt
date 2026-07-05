#ifndef SERVICE_CUSTOMIZER_H_
#define SERVICE_CUSTOMIZER_H_

#include "service.h"
#include "logic/config.h"
#include "utils/common.h"

typedef struct {
    bool hasScoreBackground;      Color scoreBackground;
    bool hasScorePlayer1;         Color scorePlayer1;
    bool hasScorePlayer2;         Color scorePlayer2;
    bool hasScorePlayer3;         Color scorePlayer3;
    bool hasScorePlayer4;         Color scorePlayer4;
    bool hasReloadWarnPrimary;    Color reloadWarnPrimary;
    bool hasReloadWarnSecondary;  Color reloadWarnSecondary;
    bool hasLowAmmoWarnPrimary;   Color lowAmmoWarnPrimary;
    bool hasLowAmmoWarnSecondary; Color lowAmmoWarnSecondary;
    bool hasNoAmmoWarnPrimary;    Color noAmmoWarnPrimary;
    bool hasNoAmmoWarnSecondary;  Color noAmmoWarnSecondary;
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
