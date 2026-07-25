#include "gui/camo/help.h"
#include "resource_ids.h"
#include <windows.h>

#define CAMO_HELP_WINDOW_TITLE "Camo Manager Help"
#define CAMO_HELP_WINDOW_WIDTH 560
#define CAMO_HELP_WINDOW_HEIGHT 520

static uiWindow *helpWindow = NULL;

static const char *HELP_TEXT = "TODO";

static int onHelpWindowClose(uiWindow *window, void *data) {
    (void)data;
    uiControlHide(uiControl(window));
    return 0;
}

static void buildHelpWindow(void) {
    helpWindow = uiNewWindow(CAMO_HELP_WINDOW_TITLE, CAMO_HELP_WINDOW_WIDTH,
                             CAMO_HELP_WINDOW_HEIGHT, 0);
    uiWindowOnClosing(helpWindow, onHelpWindowClose, NULL);
    uiWindowSetMargined(helpWindow, 1);
    uiWindowSetIcon(helpWindow, IDI_ICON1);

    uiMultilineEntry *text = uiNewMultilineEntry();
    uiMultilineEntrySetText(text, HELP_TEXT);
    uiMultilineEntrySetReadOnly(text, 1);
    uiWindowSetChild(helpWindow, uiControl(text));
}

void uiCamoHelpShow(uiWindow *parent) {
    (void)parent;

    if (helpWindow == NULL) buildHelpWindow();

    uiControlShow(uiControl(helpWindow));
    SetForegroundWindow((HWND)uiControlHandle(uiControl(helpWindow)));
}
