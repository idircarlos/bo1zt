#ifndef WIDGET_MANAGER_H_
#define WIDGET_MANAGER_H_

#include "widget.h"
#include "utils/common.h" // Rect

typedef struct WidgetManager WidgetManager;
typedef struct Controller Controller;

int widgetCount(void);
// Stable config/INI name (e.g. "Timer", "RoundTimer") for the widget at index,
// or NULL if out of range.
const char *widgetName(int index);
Rect widgetDefaultRect(int index);
int widgetDefaultFontSize(int index);

// Build the overlay runtime. Returns NULL on failure.
WidgetManager *widgetManagerCreate(Controller *controller);
void widgetManagerDestroy(WidgetManager *manager);

// Re-read config[index] and apply it live to the overlay. Called by the service
// on PATCH / reset so a config change applies instantly.
void widgetManagerApply(WidgetManager *manager, int index);

// Per-frame housekeeping: toggles overlay visibility on game-state changes and
// persists rect/font-size once an ALT-drag/resize finishes.
void widgetManagerUpdate(WidgetManager *manager);

Widget *widgetManagerGetCycleWidget(WidgetManager *manager);
// Cycle overlay of the current process-wide manager, for call sites that don't
// hold the manager (logic/game.c). NULL if no manager was created.
Widget *widgetManagerCurrentCycleWidget(void);

#endif // WIDGET_MANAGER_H_
