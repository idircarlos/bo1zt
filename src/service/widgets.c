#include "service/widgets.h"
#include "service/service_internal.h"
#include "controller.h"
#include "logic/widget/manager.h"

#include <stdio.h>
#include <string.h>

// Stable API name + label per widget, indexed to match the engine's widget
// config array order (timer, round-timer, velocity, cycle, zombies, entities).
typedef struct {
    const char *name;
    const char *label;
} WidgetEntry;

static const WidgetEntry WIDGET_TABLE[] = {
    { "timer",       "Timer" },
    { "round-timer", "Round Timer" },
    { "velocity",    "Velocity" },
    { "cycle",       "Powerup Cycle" },
    { "zombies",     "Zombies Left" },
    { "entities",    "Entities" },
};

static const int WIDGET_TABLE_SIZE = (int)(sizeof(WIDGET_TABLE) / sizeof(WIDGET_TABLE[0]));

int serviceWidgetCount(void) {
    return WIDGET_TABLE_SIZE;
}

const char *serviceWidgetNameAt(int index) {
    if (index < 0 || index >= WIDGET_TABLE_SIZE) return NULL;
    return WIDGET_TABLE[index].name;
}

const char *serviceWidgetLabelAt(int index) {
    if (index < 0 || index >= WIDGET_TABLE_SIZE) return NULL;
    return WIDGET_TABLE[index].label;
}

int serviceWidgetIndexOf(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < WIDGET_TABLE_SIZE; i++) {
        if (strcmp(WIDGET_TABLE[i].name, name) == 0) return i;
    }
    return -1;
}

ServiceResult serviceWidgetGet(Service *service, int index, WidgetConfig *configOut) {
    if (!service || !configOut) return SERVICE_INVALID_PARAM;
    if (index < 0 || index >= WIDGET_TABLE_SIZE) return SERVICE_NOT_FOUND;
    *configOut = controllerGetWidgetConfig(service->controller, index);
    return SERVICE_OK;
}

ServiceResult serviceWidgetReset(Service *service, int index) {
    if (!service) return SERVICE_INVALID_PARAM;
    if (index < 0 || index >= WIDGET_TABLE_SIZE) return SERVICE_NOT_FOUND;
    controllerResetWidgetConfig(service->controller, index);
    widgetManagerApply(controllerGetWidgetManager(service->controller), index);
    return SERVICE_OK;
}

ServiceResult serviceWidgetUpdate(Service *service, int index, const WidgetPatch *patch) {
    if (!service || !patch) return SERVICE_INVALID_PARAM;
    if (index < 0 || index >= WIDGET_TABLE_SIZE) return SERVICE_NOT_FOUND;
    Config *config = controllerGetConfig(service->controller);
    if (!config) return SERVICE_ENGINE_FAILED;
    WidgetConfig *w = &config->widgets[index];

    if (patch->hasEnabled)         w->enabled = patch->enabled;
    if (patch->hasFont)            snprintf(w->font, sizeof(w->font), "%s", patch->font ? patch->font : "");
    if (patch->hasFontSize)        w->fontSize = patch->fontSize;
    if (patch->hasTextColor)       w->textColor = patch->textColor;
    if (patch->hasHideOutsideGame) w->hideOutsideGame = patch->hideOutsideGame;
    if (patch->hasRect)            w->rect = patch->rect;

    configSave(config);
    widgetManagerApply(controllerGetWidgetManager(service->controller), index);
    return SERVICE_OK;
}
