#include "gui/twitch.h"
#include "gui/twitch/help.h"
#include "client/twitch.h"
#include "resource_ids.h"

#include <windows.h>
#include <ui.h>
#include <stdio.h>

#define TWITCH_WINDOW_TITLE "Twitch Integration"
#define TWITCH_WINDOW_WIDTH 420
#define TWITCH_WINDOW_HEIGHT 300
#define TWITCH_REFRESH_INTERVAL_MS 1000
#define TWITCH_INFO_ROWS 2
#define TWITCH_OPTION_COUNT 3

static const char *TWITCH_OPTION_KEYS[TWITCH_OPTION_COUNT] = {
    "show-chat", "send-chat", "announce-raids",
};
static const char *TWITCH_OPTION_LABELS[TWITCH_OPTION_COUNT] = {
    " Show Twitch chat in the game chat",
    " Send the game chat to Twitch",
    " Announce raids on screen",
};

static Client *client = NULL;
static uiWindow *twitchWindow = NULL;

static uiEntry *clientIdEntry = NULL;
static uiLabel *infoKeys[TWITCH_INFO_ROWS] = {NULL, NULL};
static uiLabel *infoValues[TWITCH_INFO_ROWS] = {NULL, NULL};
static uiButton *actionButton = NULL;
static uiCheckbox *optionCheckboxes[TWITCH_OPTION_COUNT] = {NULL, NULL, NULL};
static ClientTwitchState shownState = CLIENT_TWITCH_DISCONNECTED;

static void setInfoRow(int row, const char *key, const char *value) {
    uiLabelSetText(infoKeys[row], key);
    uiLabelSetText(infoValues[row], value);
}

static void showConnection(const ClientTwitchConnection *connection) {
    char status[96];

    switch (connection->state) {
        case CLIENT_TWITCH_CONNECTED:
            snprintf(status, sizeof(status), "Connected as %s", connection->login);
            setInfoRow(0, "Status:", status);
            setInfoRow(1, "", "");
            break;
        case CLIENT_TWITCH_AWAITING_AUTHORIZATION:
            setInfoRow(0, "Status:", "Waiting for authorization...");
            setInfoRow(1, "Code:", connection->userCode);
            break;
        case CLIENT_TWITCH_CONNECTING:
            setInfoRow(0, "Status:", "Connecting...");
            setInfoRow(1, "", "");
            break;
        default:
            setInfoRow(0, "Status:", "Not connected");
            setInfoRow(1, "", "");
            break;
    }

    if (connection->error[0] != '\0') setInfoRow(1, "", connection->error);

    shownState = connection->state;
    bool idle = shownState == CLIENT_TWITCH_DISCONNECTED;
    uiEntrySetReadOnly(clientIdEntry, !idle);
    uiButtonSetText(actionButton, idle ? "Connect" : "Disconnect");
}

static void refresh(void) {
    ClientTwitchConnection connection;
    if (clientGetTwitchConnection(client, &connection) != CLIENT_OK) return;
    showConnection(&connection);
}

static int onRefreshTimer(void *data) {
    (void)data;
    if (twitchWindow && uiControlVisible(uiControl(twitchWindow))) refresh();
    return 1;
}

static int onTwitchWindowClose(uiWindow *window, void *data) {
    (void)data;
    uiControlHide(uiControl(window));
    return 0;
}

static void onConnectClicked(void) {
    char *clientId = uiEntryText(clientIdEntry);
    if (!clientId || clientId[0] == '\0') {
        if (clientId) uiFreeText(clientId);
        uiMsgBoxError(twitchWindow, TWITCH_WINDOW_TITLE,
                      "Paste the Client ID of your Twitch application first.");
        return;
    }

    ClientResult result = clientTwitchConnect(client, clientId);
    uiFreeText(clientId);
    if (result != CLIENT_OK) {
        uiMsgBoxError(twitchWindow, TWITCH_WINDOW_TITLE, clientLastErrorMessage(client));
        return;
    }
    refresh();
}

static void onDisconnectClicked(void) {
    if (clientTwitchDisconnect(client) != CLIENT_OK) {
        uiMsgBoxError(twitchWindow, TWITCH_WINDOW_TITLE, clientLastErrorMessage(client));
        return;
    }
    refresh();
}

static void onActionClicked(uiButton *button, void *data) {
    (void)button;
    (void)data;
    if (shownState == CLIENT_TWITCH_DISCONNECTED) onConnectClicked();
    else onDisconnectClicked();
}

static void onHelpClicked(uiButton *button, void *data) {
    (void)button;
    (void)data;
    uiTwitchHelpShow(twitchWindow);
}

static uiControl *buildConnectionGroup(void) {
    uiGroup *group = uiNewGroup("Connection");
    uiGroupSetMargined(group, 1);

    uiGrid *grid = uiNewGrid();
    uiGridSetPadded(grid, 1);

    clientIdEntry = uiNewPasswordEntry();
    uiGridAppend(grid, uiControl(uiNewLabel("Client ID:")), 0, 0, 1, 1, 0, uiAlignStart, 0, uiAlignCenter);
    uiGridAppend(grid, uiControl(clientIdEntry), 1, 0, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);

    for (int row = 0; row < TWITCH_INFO_ROWS; row++) {
        infoKeys[row] = uiNewLabel("");
        infoValues[row] = uiNewLabel("");
        uiGridAppend(grid, uiControl(infoKeys[row]), 0, row + 1, 1, 1, 0, uiAlignStart, 0, uiAlignCenter);
        uiGridAppend(grid, uiControl(infoValues[row]), 1, row + 1, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);
    }

    uiBox *buttons = uiNewHorizontalBox();
    uiBoxSetPadded(buttons, 1);
    actionButton = uiNewButton("Connect");
    uiButtonOnClicked(actionButton, onActionClicked, NULL);
    uiButton *helpButton = uiNewButton("Help");
    uiButtonOnClicked(helpButton, onHelpClicked, NULL);
    uiBoxAppend(buttons, uiControl(actionButton), 1);
    uiBoxAppend(buttons, uiControl(helpButton), 1);
    uiGridAppend(grid, uiControl(buttons), 0, TWITCH_INFO_ROWS + 1, 2, 1, 1, uiAlignFill, 0, uiAlignCenter);

    uiGroupSetChild(group, uiControl(grid));
    return uiControl(group);
}

static void onOptionToggled(uiCheckbox *checkbox, void *data) {
    bool enabled = uiCheckboxChecked(checkbox) != 0;
    if (clientTwitchSetOption(client, (const char *)data, enabled) == CLIENT_OK) return;
    uiCheckboxSetChecked(checkbox, !enabled);
    uiMsgBoxError(twitchWindow, TWITCH_WINDOW_TITLE, clientLastErrorMessage(client));
}

static void showOptions(const ClientTwitchOptions *options) {
    const bool values[TWITCH_OPTION_COUNT] = {
        options->showChat, options->sendChat, options->announceRaids,
    };
    for (int i = 0; i < TWITCH_OPTION_COUNT; i++) {
        if (uiCheckboxChecked(optionCheckboxes[i]) != values[i]) {
            uiCheckboxSetChecked(optionCheckboxes[i], values[i]);
        }
    }
}

static uiControl *buildOptionsGroup(void) {
    uiGroup *group = uiNewGroup("Integration Options");
    uiGroupSetMargined(group, 1);

    uiBox *box = uiNewVerticalBox();
    uiBoxSetPadded(box, 1);
    for (int i = 0; i < TWITCH_OPTION_COUNT; i++) {
        optionCheckboxes[i] = uiNewCheckbox(TWITCH_OPTION_LABELS[i]);
        uiCheckboxOnToggled(optionCheckboxes[i], onOptionToggled, (void *)TWITCH_OPTION_KEYS[i]);
        uiBoxAppend(box, uiControl(optionCheckboxes[i]), 0);
    }
    uiGroupSetChild(group, uiControl(box));
    return uiControl(group);
}

static uiControl *buildContent(void) {
    uiGrid *grid = uiNewGrid();
    uiGridSetPadded(grid, 1);

    uiGridAppend(grid, buildConnectionGroup(), 0, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, buildOptionsGroup(),    0, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    return uiControl(grid);
}

static void buildTwitchWindow(void) {
    twitchWindow = uiNewWindow(TWITCH_WINDOW_TITLE, TWITCH_WINDOW_WIDTH, TWITCH_WINDOW_HEIGHT, 0);
    uiWindowOnClosing(twitchWindow, onTwitchWindowClose, NULL);
    uiWindowSetMargined(twitchWindow, 1);
    uiWindowSetResizeable(twitchWindow, 0);
    uiWindowSetChild(twitchWindow, buildContent());
    uiWindowSetIcon(twitchWindow, IDI_ICON1);
    uiTimer(TWITCH_REFRESH_INTERVAL_MS, onRefreshTimer, NULL);
}

void uiTwitchShow(Client *clientInstance, uiWindow *parentInstance) {
    (void)parentInstance;
    client = clientInstance;

    if (twitchWindow == NULL) buildTwitchWindow();

    ClientTwitchConnection connection;
    if (clientGetTwitchConnection(client, &connection) == CLIENT_OK) {
        if (connection.clientId[0]) uiEntrySetText(clientIdEntry, connection.clientId);
        showConnection(&connection);
    }

    ClientTwitchOptions options;
    if (clientGetTwitchOptions(client, &options) == CLIENT_OK) showOptions(&options);

    uiControlShow(uiControl(twitchWindow));
    SetForegroundWindow((HWND)uiControlHandle(uiControl(twitchWindow)));
}
