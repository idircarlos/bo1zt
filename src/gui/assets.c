#include "gui/assets.h"
#include "logic/assets.h"
#include "win/thread.h"
#include "logger.h"
#include "resource_ids.h"

#include <ui.h>

#include <stdio.h>
#include <string.h>

#define ASSETS_WINDOW_WIDTH  380
#define ASSETS_WINDOW_HEIGHT 80
#define ASSETS_POLL_MS       100

typedef struct {
    char location[512];
    int zones;
    long failed;
    volatile int current;
    volatile int cancelled;
    int done;
    uiWindow *win;
    uiProgressBar *bar;
    uiLabel *label;
    uiLabel *pct;
} AssetsInstall;

static void assetsRefresh(AssetsInstall *s) {
    int total = assetsModelTotal();
    int value = total > 0 ? (assetsExtractedCount() * 100) / total : 100;
    if (value > 100) value = 100;
    uiProgressBarSetValue(s->bar, value);

    char name[128];
    snprintf(name, sizeof(name), "Extracting %s", assetsZoneName(s->current));
    uiLabelSetText(s->label, name);

    char pct[16];
    snprintf(pct, sizeof(pct), "%d%%", value);
    uiLabelSetText(s->pct, pct);
}

static int assetsOnTimer(void *data) {
    AssetsInstall *s = (AssetsInstall *)data;
    if (s->done) return 0;
    assetsRefresh(s);
    return 1;
}

static void assetsOnFinished(void *data) {
    ((AssetsInstall *)data)->done = 1;
}

static int assetsWorker(void *data) {
    AssetsInstall *s = (AssetsInstall *)data;
    for (int i = 0; i < s->zones && !s->cancelled; i++) {
        s->current = i;
        if (!assetsExtractZone(s->location, i)) s->failed = s->failed + 1;
    }
    if (!s->cancelled) assetsCleanup();
    uiQueueMain(assetsOnFinished, s);
    return 0;
}

static int assetsOnClosing(uiWindow *w, void *data) {
    (void)w;
    ((AssetsInstall *)data)->cancelled = 1;
    return 0;
}

bool uiAssetsInstall(const char *gameLocation) {
    if (assetsInstalled()) {
        LOG_INFO("Assets already installed, skipping extraction");
        return true;
    }
    if (!gameLocation || gameLocation[0] == '\0') {
        LOG_WARN("Game location not set; skipping asset extraction");
        return false;
    }

    static AssetsInstall s;
    memset(&s, 0, sizeof(s));
    strncpy(s.location, gameLocation, sizeof(s.location) - 1);
    s.zones = assetsZoneCount();

    s.win = uiNewWindow("Extracting assets", ASSETS_WINDOW_WIDTH, ASSETS_WINDOW_HEIGHT, 0);
    uiWindowSetIcon(s.win, IDI_ICON1);
    uiWindowSetMargined(s.win, 1);
    uiWindowSetResizeable(s.win, 0);
    uiWindowOnClosing(s.win, assetsOnClosing, &s);

    uiBox *box = uiNewVerticalBox();
    uiBoxSetPadded(box, 1);

    s.label = uiNewLabel("Preparing to extract game assets...");
    uiBoxAppend(box, uiControl(s.label), 0);

    s.bar = uiNewProgressBar();
    uiBoxAppend(box, uiControl(s.bar), 0);

    s.pct = uiNewLabel("0%");
    uiGrid *pctGrid = uiNewGrid();
    uiGridAppend(pctGrid, uiControl(s.pct), 0, 0, 1, 1, 1, uiAlignCenter, 0, uiAlignFill);
    uiBoxAppend(box, uiControl(pctGrid), 0);

    uiWindowSetChild(s.win, uiControl(box));

    int screenWidth, screenHeight;
    uiScreenGetResolution(&screenWidth, &screenHeight);
    uiWindowSetPosition(s.win, screenWidth / 2 - (ASSETS_WINDOW_WIDTH / 2),
                        screenHeight / 2 - (ASSETS_WINDOW_HEIGHT / 2));
    uiControlShow(uiControl(s.win));

    LOG_INFO("Extracting game assets from '%s' into the bo1zt models folder...", gameLocation);
    Thread *worker = threadCreate(assetsWorker, &s);
    if (!worker) {
        LOG_ERROR("Failed to start asset extraction thread");
        uiControlDestroy(uiControl(s.win));
        return false;
    }

    uiTimer(ASSETS_POLL_MS, assetsOnTimer, &s);
    while (!s.done) uiMainStep(1);
    assetsRefresh(&s);

    threadWait(worker, -1);
    threadClose(worker);

    bool ok = !s.cancelled && s.failed < s.zones;
    if (ok) {
        LOG_INFO("Asset extraction complete");
    } else if (s.cancelled) {
        LOG_WARN("Asset extraction cancelled by user");
    } else {
        LOG_ERROR("Asset extraction failed for every zone; check the game location");
    }

    uiControlDestroy(uiControl(s.win));
    return ok;
}
