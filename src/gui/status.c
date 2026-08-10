#include "gui/status.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "gui.h"
#include "logger.h"
#include "resource_ids.h"
#include "client/camo.h"
#include "client/twitch.h"
#include "win/http.h"
#include "win/resources.h"

#define TWITCH_ICON_HOST_SIZE 128
#define TWITCH_ICON_PATH_SIZE 256
#define TWITCH_ICON_URL_SIZE 256

#define GAME_EXECUTABLE_NAME "BlackOps.exe"
#define GAME_ICON_PATH_SIZE (MAX_PATH + sizeof(GAME_EXECUTABLE_NAME) + 1)

#define BUNDLE_TEXT_PREFIX "Camo Bundle: "
#define BUNDLE_TEXT_SIZE (sizeof(BUNDLE_TEXT_PREFIX) + CLIENT_CAMO_NAME_SIZE)

typedef enum {
    GAME_ACTIVITY_NOT_RUNNING,
    GAME_ACTIVITY_OPENING,
    GAME_ACTIVITY_RUNNING,
    GAME_ACTIVITY_CLOSING
} GameActivity;

static const char *GAME_ACTIVITY_TABLE[] = {
    "Not running",
    "Opening",
    "Running",
    "Closing",
};

static uiStatusBar *statusBar = NULL;
static uiWindow *statusWindow = NULL;
static int itemCount = 0;
static int spreadWidth = 0;
static int gameItem = 0;
static int twitchItem = 0;
static int bundleItem = 0;
static char twitchIconUrl[TWITCH_ICON_PATH_SIZE] = "";
static char gameIconPath[GAME_ICON_PATH_SIZE] = "";
static GameActivity gameActivity = GAME_ACTIVITY_NOT_RUNNING;
static uiImage *gameIcon = NULL;
static uiImage *twitchIcon = NULL;

static bool splitUrl(const char *url, char *host, size_t hostSize, char *path, size_t pathSize) {
    const char *authority = strstr(url, "://");
    authority = authority ? authority + 3 : url;

    const char *separator = strchr(authority, '/');
    if (!separator) return false;

    size_t hostLength = (size_t)(separator - authority);
    if (hostLength == 0 || hostLength >= hostSize) return false;

    memcpy(host, authority, hostLength);
    host[hostLength] = '\0';
    snprintf(path, pathSize, "%s", separator);
    return true;
}

static void *twitchIconDownload(const char *url, size_t *outSize) {
    char host[TWITCH_ICON_HOST_SIZE];
    char path[TWITCH_ICON_PATH_SIZE];

    if (!url || !url[0] || !outSize) return NULL;
    if (!splitUrl(url, host, sizeof(host), path, sizeof(path))) {
        LOG_WARN("Twitch Icon: %s is not a usable url", url);
        return NULL;
    }

    HttpClientResponse response = httpsClientRequest(host, 443, "GET", path, NULL, NULL);
    if (response.status != 200 || response.size == 0) {
        LOG_WARN("Twitch Icon: %s could not be downloaded (%d)", url, response.status);
        httpClientResponseFree(&response);
        return NULL;
    }

    *outSize = response.size;
    return response.body;
}

static void updateBundleItem(void) {
    ClientCamoOverview overview;
    ClientCamoBundle bundle;

    if (clientGetCamoOverview(guiClient(), &overview) != CLIENT_OK) return;

    if (!overview.activeBundleId[0] ||
        clientGetCamoBundle(guiClient(), overview.activeBundleId, &bundle) != CLIENT_OK) {
        uiStatusBarSetItemText(statusBar, bundleItem, "No Camo Bundle installed");
        return;
    }

    char text[BUNDLE_TEXT_SIZE];
    snprintf(text, sizeof(text), BUNDLE_TEXT_PREFIX "%s", bundle.name);
    uiStatusBarSetItemText(statusBar, bundleItem, text);
    clientFreeCamoBundle(&bundle);
}

static void updateTwitchItem(void) {
    ClientTwitchConnection twitch;
    if (clientGetTwitchConnection(guiClient(), &twitch) != CLIENT_OK) return;

    bool connected = twitch.state == CLIENT_TWITCH_CONNECTED;
    if (connected && twitch.displayName[0]) {
        char text[sizeof(twitch.displayName) + 16];
        snprintf(text, sizeof(text), "Connected as %s", twitch.displayName);
        uiStatusBarSetItemText(statusBar, twitchItem, text);
    } else {
        uiStatusBarSetItemText(statusBar, twitchItem, "Twitch not connected");
    }

    const char *url = connected ? twitch.profileImageUrl : "";
    if (strcmp(twitchIconUrl, url) == 0) return;
    snprintf(twitchIconUrl, sizeof(twitchIconUrl), "%s", url);

    size_t size = 0;
    void *data = twitchIconDownload(url, &size);
    uiImage *icon = data ? uiNewImageFromData(data, size) : NULL;
    free(data);

    if (icon)
        uiStatusBarSetItemImage(statusBar, twitchItem, icon, true);
    else
        uiStatusBarClearItemIcon(statusBar, twitchItem);

    if (twitchIcon) uiFreeImage(twitchIcon);
    twitchIcon = icon;
}

static GameActivity gameActivityNext(GameActivity current, const GuiSnapshot *snapshot) {
    if (!snapshot->statusValid) return GAME_ACTIVITY_NOT_RUNNING;

    bool running = snapshot->status.running;
    bool attached = snapshot->status.attached;
    bool ready = snapshot->status.ready;

    switch (current) {
        case GAME_ACTIVITY_NOT_RUNNING:
            if (ready) return GAME_ACTIVITY_RUNNING;
            return running ? GAME_ACTIVITY_OPENING : GAME_ACTIVITY_NOT_RUNNING;
        case GAME_ACTIVITY_OPENING:
            if (ready) return GAME_ACTIVITY_RUNNING;
            if (running) return GAME_ACTIVITY_OPENING;
            return attached ? GAME_ACTIVITY_CLOSING : GAME_ACTIVITY_NOT_RUNNING;
        case GAME_ACTIVITY_RUNNING:
            if (running) return GAME_ACTIVITY_RUNNING;
            return attached ? GAME_ACTIVITY_CLOSING : GAME_ACTIVITY_NOT_RUNNING;
        case GAME_ACTIVITY_CLOSING:
            if (running) return GAME_ACTIVITY_OPENING;
            return attached ? GAME_ACTIVITY_CLOSING : GAME_ACTIVITY_NOT_RUNNING;
    }
    return GAME_ACTIVITY_NOT_RUNNING;
}

static void gameItemSetActivity(GameActivity activity) {
    uiStatusBarSetItemText(statusBar, gameItem, GAME_ACTIVITY_TABLE[activity]);
}

static void updateGameIcon(const GuiSnapshot *snapshot) {
    char path[GAME_ICON_PATH_SIZE];

    if (!snapshot->gameConfigValid || !snapshot->gameConfig.location[0]) return;

    snprintf(path, sizeof(path), "%s\\%s", snapshot->gameConfig.location, GAME_EXECUTABLE_NAME);
    if (strcmp(gameIconPath, path) == 0) return;
    snprintf(gameIconPath, sizeof(gameIconPath), "%s", path);

    uiImage *icon = uiNewImageFromFileIcon(path);
    if (!icon) {
        LOG_WARN("Status: no icon could be read from %s", path);
        return;
    }
    uiStatusBarSetItemImage(statusBar, gameItem, icon, false);
    if (gameIcon) uiFreeImage(gameIcon);
    gameIcon = icon;
}

static int statusAppendItem(const char *text) {
    itemCount++;
    return uiStatusBarAppend(statusBar, text);
}

static void statusSpreadItems(void) {
    int width = 0;
    int height = 0;

    if (!statusWindow || itemCount == 0) return;

    uiWindowContentSize(statusWindow, &width, &height);
    if (width <= 0 || width == spreadWidth) return;
    spreadWidth = width;

    int itemWidth = width / itemCount;
    for (int item = 0; item < itemCount; item++)
        uiStatusBarSetItemWidth(statusBar, item, itemWidth, itemWidth);
}

void uiStatusBuild(uiWindow *window) {
    void *icon = NULL;
    uint32_t iconSize = 0;

    statusBar = uiNewStatusBar(false);
    statusWindow = window;
    gameItem = statusAppendItem("");
    twitchItem = statusAppendItem("Twitch not connected");
    bundleItem = statusAppendItem("No Camo Bundle installed");

    if (resourcesGetData(IDR_PNG_ICON, &icon, &iconSize)) {
        gameIcon = uiNewImageFromData(icon, iconSize);
        if (gameIcon) uiStatusBarSetItemImage(statusBar, gameItem, gameIcon, false);
    }

    gameItemSetActivity(gameActivity);
    uiWindowSetStatusBar(window, statusBar);
    statusSpreadItems();
}

void uiStatusUpdate(void) {
    if (!statusBar) return;

    statusSpreadItems();

    const GuiSnapshot *snapshot = guiGetSnapshot();

    gameActivity = gameActivityNext(gameActivity, snapshot);
    gameItemSetActivity(gameActivity);
    updateGameIcon(snapshot);

    updateTwitchItem();
    updateBundleItem();
}
