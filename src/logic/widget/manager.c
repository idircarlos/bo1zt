#include "logic/widget/manager.h"
#include "controller.h"
#include "logic/config.h"
#include "logic/state.h"
#include "logic/game.h"
#include "widget/timer.h"
#include "widget/velocity.h"
#include "widget/cycle.h"
#include "widget/zombies.h"
#include "widget/entities.h"
#include "win/thread.h"
#include "logger.h"
#include <windows.h>
#include <stdlib.h>

#define N_WIDGETS 6

// Widget indices, matching the config array order and the API name table.
typedef enum {
    WIDGET_IDX_TIMER,
    WIDGET_IDX_ROUND_TIMER,
    WIDGET_IDX_VELOCITY,
    WIDGET_IDX_CYCLE,
    WIDGET_IDX_ZOMBIES,
    WIDGET_IDX_ENTITIES,
} WidgetIndex;

// INI/config names (kept identical to the previous GUI-side table so existing
// bo1zt.ini files keep matching).
static const char *WIDGET_CONFIG_NAMES[N_WIDGETS] = {
    "Timer", "RoundTimer", "Velocity", "Cycle", "Zombies", "Entities",
};

struct WidgetManager {
    Controller *controller;
    Config *config;
    State *state;
    Widget *widgets[N_WIDGETS];
    bool wasTransforming;
    bool wasGameRunning;
    // Dedicated core thread that owns and pumps the overlay windows. The windows
    // are created on this thread (so it owns them and runs their WndProc for
    // ALT drag/resize) and it runs a message loop for their lifetime. It belongs
    // to the core and is entirely independent of the libui UI thread — which is
    // what makes the overlays GUI-independent and removes the cross-thread
    // deadlock: this owner thread never blocks on an HTTP request, so window ops
    // issued from the server/update threads are always serviced promptly.
    Thread *overlayThread;
    DWORD overlayThreadId;
    HANDLE readyEvent; // signalled once the overlays are created
};

// Current process-wide manager, so call sites without the handle (logic/game.c)
// can reach the cycle overlay.
static WidgetManager *g_current = NULL;

// --- Metadata ---------------------------------------------------------------

int widgetCount(void) {
    return N_WIDGETS;
}

const char *widgetName(int index) {
    if (index < 0 || index >= N_WIDGETS) return NULL;
    return WIDGET_CONFIG_NAMES[index];
}

Rect widgetDefaultRect(int index) {
    switch (index) {
        case WIDGET_IDX_TIMER:
        case WIDGET_IDX_ROUND_TIMER: return WIDGET_TIMER_RECT;
        case WIDGET_IDX_VELOCITY:    return WIDGET_VELOCITY_RECT;
        case WIDGET_IDX_CYCLE:       return WIDGET_CYCLE_RECT;
        case WIDGET_IDX_ZOMBIES:     return WIDGET_ZOMBIES_RECT;
        case WIDGET_IDX_ENTITIES:    return WIDGET_ENTITIES_RECT;
        default:
            LOG_ERROR("Unknown widget index %d", index);
            return rectCreate(0, 0, 0, 0);
    }
}

int widgetDefaultFontSize(int index) {
    switch (index) {
        case WIDGET_IDX_TIMER:
        case WIDGET_IDX_ROUND_TIMER: return WIDGET_TIMER_FONT_SIZE;
        case WIDGET_IDX_VELOCITY:    return WIDGET_VELOCITY_FONT_SIZE;
        case WIDGET_IDX_CYCLE:       return 0; // Cycle widget doesn't use a font
        case WIDGET_IDX_ZOMBIES:     return WIDGET_ZOMBIES_FONT_SIZE;
        case WIDGET_IDX_ENTITIES:    return WIDGET_ENTITIES_FONT_SIZE;
        default:
            LOG_ERROR("Unknown widget index %d", index);
            return 0;
    }
}

static bool managerGameRunning(WidgetManager *manager) {
    if (!manager->state) return false;
    return gameRunning(&manager->state->activeGame);
}

static void applyVisibility(WidgetManager *manager, int index) {
    if (!manager->widgets[index]) return;
    WidgetConfig *w = &manager->config->widgets[index];
    bool running = managerGameRunning(manager);
    if (!w->enabled || (w->hideOutsideGame && !running)) {
        widgetHide(manager->widgets[index]);
    } else {
        widgetShow(manager->widgets[index]);
    }
}

// Safe to call from any thread: the window operations are cross-thread
// SendMessages serviced by the overlay owner thread, which is never blocked on
// an HTTP request, so there is no deadlock.
static void applyWidget(WidgetManager *manager, int index) {
    Widget *widget = manager->widgets[index];
    if (!widget) return;
    WidgetConfig *w = &manager->config->widgets[index];
    widgetSetFont(widget, w->font);
    widgetSetTextColor(widget, w->textColor);
    widgetSetPosition(widget, w->rect);
    widgetSetFontSize(widget, w->fontSize);
    applyVisibility(manager, index);
}

void widgetManagerApply(WidgetManager *manager, int index) {
    if (!manager || index < 0 || index >= N_WIDGETS) return;
    applyWidget(manager, index);
}

// --- Lifecycle ---------------------------------------------------------------

static int overlayThreadProc(void *data) {
    WidgetManager *manager = (WidgetManager *)data;
    manager->overlayThreadId = GetCurrentThreadId();

    Game *activeGame = &manager->state->activeGame;
    manager->widgets[WIDGET_IDX_TIMER]       = timerWidgetCreate(&activeGame->elapsed);
    manager->widgets[WIDGET_IDX_ROUND_TIMER] = timerWidgetCreate(&activeGame->currentRound.elapsed);
    manager->widgets[WIDGET_IDX_VELOCITY]    = velocityWidgetCreate(&activeGame->movementSpeed);
    manager->widgets[WIDGET_IDX_CYCLE]       = cycleWidgetCreate();
    manager->widgets[WIDGET_IDX_ZOMBIES]     = zombiesWidgetCreate(&activeGame->currentRound.zombiesLeft);
    manager->widgets[WIDGET_IDX_ENTITIES]    = entitiesWidgetCreate(&activeGame->currentEntities, &activeGame->maxEntities);

    for (int i = 0; i < N_WIDGETS; i++) {
        applyWidget(manager, i);
    }
    manager->wasGameRunning = managerGameRunning(manager);

    // Force this thread to have a message queue before signalling readiness so
    // cross-thread window ops are serviced from the moment creation returns.
    MSG msg;
    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
    if (manager->readyEvent) SetEvent(manager->readyEvent);

    // Own + pump the overlay windows, independent of the libui UI thread.
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

WidgetManager *widgetManagerCreate(Controller *controller) {
    if (!controller) return NULL;
    WidgetManager *manager = (WidgetManager *)calloc(1, sizeof(WidgetManager));
    if (!manager) {
        LOG_ERROR("Couldn't allocate WidgetManager");
        return NULL;
    }
    manager->controller = controller;
    manager->config = controllerGetConfig(controller);
    manager->state = controllerGetState(controller);
    manager->wasTransforming = false;
    manager->wasGameRunning = false;
    manager->readyEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // manual-reset
    g_current = manager;

    // The overlay windows must be created on their owner thread (Win32 window
    // thread affinity), so creation happens inside the overlay thread. Wait for
    // it to finish so the manager is fully usable when we return.
    manager->overlayThread = threadCreate(overlayThreadProc, manager);
    if (!manager->overlayThread) {
        LOG_ERROR("Couldn't start widget overlay thread");
    } else if (manager->readyEvent) {
        WaitForSingleObject(manager->readyEvent, 5000);
    }
    return manager;
}

void widgetManagerDestroy(WidgetManager *manager) {
    if (!manager) return;
    for (int i = 0; i < N_WIDGETS; i++) {
        if (manager->widgets[i]) widgetDestroy(manager->widgets[i]);
    }
    // Stop the overlay pump thread.
    if (manager->overlayThreadId) PostThreadMessage(manager->overlayThreadId, WM_QUIT, 0, 0);
    if (manager->overlayThread) {
        threadWait(manager->overlayThread, 2000);
        threadClose(manager->overlayThread);
    }
    if (manager->readyEvent) CloseHandle(manager->readyEvent);
    if (g_current == manager) g_current = NULL;
    free(manager);
}

void widgetManagerUpdate(WidgetManager *manager) {
    if (!manager) return;

    // Persist rect/font-size once an ALT-drag/resize finishes.
    bool transformingNow = false;
    for (int i = 0; i < N_WIDGETS; i++) {
        if (manager->widgets[i] && widgetIsTransforming(manager->widgets[i])) {
            transformingNow = true;
            break;
        }
    }
    if (manager->wasTransforming && !transformingNow) {
        for (int i = 0; i < N_WIDGETS; i++) {
            if (!manager->widgets[i]) continue;
            manager->config->widgets[i].rect = widgetGetPosition(manager->widgets[i]);
            manager->config->widgets[i].fontSize = widgetGetFontSize(manager->widgets[i]);
        }
        configSave(manager->config);
    }
    manager->wasTransforming = transformingNow;

    // Show/hide on game-state transitions.
    bool running = managerGameRunning(manager);
    if (running != manager->wasGameRunning) {
        for (int i = 0; i < N_WIDGETS; i++) {
            applyVisibility(manager, i);
        }
        manager->wasGameRunning = running;
    }
}

Widget *widgetManagerGetCycleWidget(WidgetManager *manager) {
    if (!manager) return NULL;
    return manager->widgets[WIDGET_IDX_CYCLE];
}

Widget *widgetManagerCurrentCycleWidget(void) {
    return widgetManagerGetCycleWidget(g_current);
}
