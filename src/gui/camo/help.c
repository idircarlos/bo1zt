#include "gui/camo/help.h"
#include "win/resources.h"
#include "resource_ids.h"
#include "logger.h"

#include <windows.h>
#include <stdlib.h>
#include <string.h>

#define CAMO_HELP_WINDOW_TITLE "Camo Manager Help"
#define CAMO_HELP_WINDOW_WIDTH 560
#define CAMO_HELP_WINDOW_HEIGHT 520

static uiWindow *helpWindow = NULL;

static int onHelpWindowClose(uiWindow *window, void *data) {
    (void)data;
    uiControlHide(uiControl(window));
    return 0;
}

static char *loadHelpMarkdown(void) {
    void *data = NULL;
    DWORD size = 0;
    if (!resourcesGetData(IDR_MARKDOWN_CAMO_MANAGER, &data, &size)) {
        LOG_ERROR("Camo help: missing markdown resource");
        return NULL;
    }

    char *markdown = (char *)malloc(size + 1);
    if (markdown == NULL) return NULL;

    memcpy(markdown, data, size);
    markdown[size] = '\0';
    return markdown;
}

static void buildHelpWindow(void) {
    helpWindow = uiNewWindow(CAMO_HELP_WINDOW_TITLE, CAMO_HELP_WINDOW_WIDTH,
                             CAMO_HELP_WINDOW_HEIGHT, 0);
    uiWindowOnClosing(helpWindow, onHelpWindowClose, NULL);
    uiWindowSetMargined(helpWindow, 1);
    uiWindowSetIcon(helpWindow, IDI_ICON1);

    uiMarkdownViewer *text = uiNewMarkdownViewer();
    char *markdown = loadHelpMarkdown();
    if (markdown != NULL) {
        uiMarkdownViewerSetText(text, markdown);
        free(markdown);
    }
    uiWindowSetChild(helpWindow, uiControl(text));
}

void uiCamoHelpShow(uiWindow *parent) {
    (void)parent;

    if (helpWindow == NULL) buildHelpWindow();

    uiControlShow(uiControl(helpWindow));
    SetForegroundWindow((HWND)uiControlHandle(uiControl(helpWindow)));
}
