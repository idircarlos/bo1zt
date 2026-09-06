#include "gui/logs.h"

#include <windows.h>
#include <shellapi.h>
#include <ui.h>
#include <ui_windows.h>

#include <stdio.h>
#include <string.h>

#include "gui.h"
#include "gui/logs/view.h"
#include "logger.h"
#include "resource_ids.h"
#include "win/file.h"

#define LOGS_WINDOW_TITLE "Logs"
#define LOGS_WINDOW_WIDTH 940
#define LOGS_WINDOW_HEIGHT 560
#define LOGS_REFRESH_INTERVAL_MS 400

#define LOGS_EXE_FILE "bo1zt.log"
#define LOGS_DLL_FILE "bo1zt_dll.log"

static const char *const LOGS_LEVEL_NAME[] = {
    "Trace", "Debug", "Info", "Warn", "Error", "Fatal"
};

#define LOGS_FILTER_COUNT (sizeof(LOGS_LEVEL_NAME) / sizeof(*LOGS_LEVEL_NAME))

static uiWindow *logsWindow = NULL;
static uiTab *tabs = NULL;
static uiRadioButtons *levelRadio = NULL;

static LogsView *exeView = NULL;
static LogsView *dllView = NULL;
static bool dllTailing = false;

static LogsView *activeView(void) {
    return uiTabSelected(tabs) == 1 ? dllView : exeView;
}

// The DLL log is only meaningful once the DLL of this session is injected:
// anything already in the file belongs to a previous game.
static void refreshDllPath(void) {
    if (dllTailing) return;

    const GuiSnapshot *snapshot = guiGetSnapshot();
    if (!snapshot->statusValid || !snapshot->status.dllInjected) return;

    char folder[MAX_PATH];
    if (!loggerFolder(folder, sizeof(folder))) return;

    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\%s", folder, LOGS_DLL_FILE);
    logsViewSetPath(dllView, path);
    dllTailing = true;
}

static void refresh(void) {
    refreshDllPath();
    logsViewRefresh(exeView);
    logsViewRefresh(dllView);
}

static int onRefreshTimer(void *data) {
    (void)data;
    if (logsWindow && uiControlVisible(uiControl(logsWindow))) refresh();
    return 1;
}

static int onWindowClose(uiWindow *window, void *data) {
    (void)data;
    uiControlHide(uiControl(window));
    return 0;
}

static void onLevelSelected(uiRadioButtons *buttons, void *data) {
    (void)data;
    int index = uiRadioButtonsSelected(buttons);
    if (index < 0) return;

    logsViewSetMinimumLevel(exeView, (LogsLevel)index);
    logsViewSetMinimumLevel(dllView, (LogsLevel)index);
}

static void onOpenFolderClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)item; (void)window; (void)data;

    char folder[MAX_PATH];
    if (!loggerFolder(folder, sizeof(folder))) {
        LOG_ERROR("Logs: failed to resolve %%APPDATA%%");
        return;
    }

    fileCreateFolder(folder);
    ShellExecuteA(NULL, "open", folder, NULL, NULL, SW_SHOWNORMAL);
}

static void onClearClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)item; (void)window; (void)data;
    logsViewClear(activeView());
}

static void onCopyClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)item; (void)window; (void)data;
    logsViewCopy(activeView());
}

static void onSelectAllClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)item; (void)window; (void)data;
    logsViewSelectAll(activeView());
}

static void onCloseClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)item; (void)window; (void)data;
    uiControlHide(uiControl(logsWindow));
}

static void onAutoscrollClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)window; (void)data;
    bool enabled = uiMenuItemChecked(item) != 0;

    logsViewSetAutoscroll(exeView, enabled);
    logsViewSetAutoscroll(dllView, enabled);
}

static void onCurrentRunOnlyClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)window; (void)data;
    bool enabled = uiMenuItemChecked(item) != 0;

    logsViewSetCurrentRunOnly(exeView, enabled);
    logsViewSetCurrentRunOnly(dllView, enabled);
}

static void onWordWrapClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)window; (void)data;
    bool enabled = uiMenuItemChecked(item) != 0;

    logsViewSetWordWrap(exeView, enabled);
    logsViewSetWordWrap(dllView, enabled);
}

static void onHideTimestampClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)window; (void)data;
    bool hidden = uiMenuItemChecked(item) != 0;

    logsViewSetHideTimestamp(exeView, hidden);
    logsViewSetHideTimestamp(dllView, hidden);
}

static void onHideOriginClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)window; (void)data;
    bool hidden = uiMenuItemChecked(item) != 0;

    logsViewSetHideOrigin(exeView, hidden);
    logsViewSetHideOrigin(dllView, hidden);
}

static void onZoomInClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)item; (void)window; (void)data;
    logsViewZoomIn(activeView());
}

static void onZoomOutClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)item; (void)window; (void)data;
    logsViewZoomOut(activeView());
}

static void onZoomResetClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)item; (void)window; (void)data;
    logsViewZoomReset(activeView());
}

static uiControl *buildLevelBar(void) {
    levelRadio = uiNewRadioButtons(1);
    for (size_t i = 0; i < LOGS_FILTER_COUNT; i++) {
        uiRadioButtonsAppend(levelRadio, LOGS_LEVEL_NAME[i]);
    }
    uiRadioButtonsSetSelected(levelRadio, 0);
    uiRadioButtonsOnSelected(levelRadio, onLevelSelected, NULL);

    uiGroup *group = uiNewGroup("Minimum level");
    uiGroupSetMargined(group, 1);
    uiGroupSetChild(group, uiControl(levelRadio));

    return uiControl(group);
}

static uiMenuBar *buildMenuBar(void) {
    uiMenuBar *bar = uiNewMenuBar();

    uiMenu *file = uiMenuBarAppendMenu(bar, "File");
    uiMenuItem *openFolder = uiMenuAppendItem(file, "Open Logs Folder");
    uiMenuItemOnClicked(openFolder, onOpenFolderClicked, NULL);
    uiMenuItemSetShortcut(openFolder, uiModifierCtrl | uiModifierShift, 'O');

    uiMenuAppendSeparator(file);
    uiMenuItem *close = uiMenuAppendItem(file, "Close");
    uiMenuItemOnClicked(close, onCloseClicked, NULL);
    uiMenuItemSetShortcut(close, uiModifierCtrl, 'W');

    uiMenu *edit = uiMenuBarAppendMenu(bar, "Edit");
    uiMenuItem *clear = uiMenuAppendItem(edit, "Clear");
    uiMenuItemOnClicked(clear, onClearClicked, NULL);
    uiMenuItemSetShortcut(clear, uiModifierCtrl, 'L');

    uiMenuItem *copy = uiMenuAppendItem(edit, "Copy");
    uiMenuItemOnClicked(copy, onCopyClicked, NULL);
    uiMenuItemSetShortcut(copy, uiModifierCtrl, 'C');

    uiMenuItem *selectAll = uiMenuAppendItem(edit, "Select All");
    uiMenuItemOnClicked(selectAll, onSelectAllClicked, NULL);
    uiMenuItemSetShortcut(selectAll, uiModifierCtrl, 'A');

    uiMenu *view = uiMenuBarAppendMenu(bar, "View");
    uiMenuItem *currentRunOnly = uiMenuAppendCheckItem(view, "Current Run Only");
    uiMenuItemOnClicked(currentRunOnly, onCurrentRunOnlyClicked, NULL);
    uiMenuItemSetChecked(currentRunOnly, 1);

    uiMenuAppendSeparator(view);
    uiMenuItem *autoscroll = uiMenuAppendCheckItem(view, "Autoscroll");
    uiMenuItemOnClicked(autoscroll, onAutoscrollClicked, NULL);
    uiMenuItemSetChecked(autoscroll, 1);

    uiMenuItem *wordWrap = uiMenuAppendCheckItem(view, "Word Wrap");
    uiMenuItemOnClicked(wordWrap, onWordWrapClicked, NULL);
    uiMenuItemSetShortcut(wordWrap, uiModifierAlt, 'Z');

    uiMenuAppendSeparator(view);
    uiMenuItem *hideTimestamp = uiMenuAppendCheckItem(view, "Hide Timestamp");
    uiMenuItemOnClicked(hideTimestamp, onHideTimestampClicked, NULL);

    uiMenuItem *hideOrigin = uiMenuAppendCheckItem(view, "Hide Origin");
    uiMenuItemOnClicked(hideOrigin, onHideOriginClicked, NULL);

    uiMenuAppendSeparator(view);
    uiMenu *zoom = uiMenuAppendSubmenu(view, "Zoom");
    uiMenuItem *zoomIn = uiMenuAppendItem(zoom, "Zoom In    (Ctrl+Mouse Wheel Up)");
    uiMenuItemOnClicked(zoomIn, onZoomInClicked, NULL);
    uiMenuItemSetShortcut(zoomIn, uiModifierCtrl, uiExtKeyNAdd);

    uiMenuItem *zoomOut = uiMenuAppendItem(zoom, "Zoom Out (Ctrl+Mouse Wheel Down)");
    uiMenuItemOnClicked(zoomOut, onZoomOutClicked, NULL);
    uiMenuItemSetShortcut(zoomOut, uiModifierCtrl, uiExtKeyNSubtract);

    uiMenuItem *zoomReset = uiMenuAppendItem(zoom, "Restore Default Zoom");
    uiMenuItemOnClicked(zoomReset, onZoomResetClicked, NULL);
    uiMenuItemSetShortcut(zoomReset, uiModifierCtrl, uiExtKeyNDivide);

    return bar;
}

static void setExePath(void) {
    char folder[MAX_PATH];
    if (!loggerFolder(folder, sizeof(folder))) return;

    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\%s", folder, LOGS_EXE_FILE);
    logsViewSetPath(exeView, path);
}

static uiControl *buildContent(void) {
    exeView = logsViewCreate();
    dllView = logsViewCreate();
    setExePath();

    tabs = uiNewTab();
    uiTabAppend(tabs, "bo1zt", logsViewControl(exeView));
    uiTabAppend(tabs, "DLL", logsViewControl(dllView));
    uiTabSetMargined(tabs, 0, 1);
    uiTabSetMargined(tabs, 1, 1);

    uiBox *rows = uiNewVerticalBox();
    uiBoxSetPadded(rows, 1);
    uiBoxAppend(rows, buildLevelBar(), 0);
    uiBoxAppend(rows, uiControl(tabs), 1);

    return uiControl(rows);
}

static void buildLogsWindow(void) {
    logsWindow = uiNewWindow(LOGS_WINDOW_TITLE, LOGS_WINDOW_WIDTH, LOGS_WINDOW_HEIGHT, 0);
    uiWindowOnClosing(logsWindow, onWindowClose, NULL);
    uiWindowSetMargined(logsWindow, 1);
    uiWindowSetChild(logsWindow, buildContent());
    uiWindowSetMenuBar(logsWindow, buildMenuBar());
    uiWindowSetIcon(logsWindow, IDI_ICON1);
    uiTimer(LOGS_REFRESH_INTERVAL_MS, onRefreshTimer, NULL);
}

void uiLogsShow(void) {
    if (logsWindow == NULL) buildLogsWindow();

    refresh();
    uiControlShow(uiControl(logsWindow));
    SetForegroundWindow((HWND)uiControlHandle(uiControl(logsWindow)));
}
