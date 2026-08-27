#include "gui/gsc/editor.h"

#include <Scintilla.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client/gsc.h"
#include "gui/gsc/lexer.h"

#define GSC_TAB_MAX 16
#define GSC_EDITOR_FONT "Consolas"
#define GSC_EDITOR_FONT_SIZE 10
#define GSC_EDITOR_TAB_WIDTH 4
#define GSC_AUTOSAVE_INTERVAL 800

typedef struct {
    char path[CLIENT_GSC_PATH_SIZE];
    uiScintilla *editor;
    bool dirty;
} GscTab;

static Client *client = NULL;
static uiWindow *host = NULL;
static uiTab *tabs = NULL;

static GscTab openTabs[GSC_TAB_MAX];
static size_t tabCount = 0;

static GscTab *tabOf(uiScintilla *editor) {
    for (size_t i = 0; i < tabCount; i++) {
        if (openTabs[i].editor == editor) return &openTabs[i];
    }
    return NULL;
}

static void saveTab(GscTab *tab) {
    size_t length = (size_t)uiScintillaSend(tab->editor, SCI_GETLENGTH, 0, 0);

    char *text = (char *)malloc(length + 1);
    if (!text) return;
    uiScintillaSend(tab->editor, SCI_GETTEXT, length + 1, (intptr_t)text);

    if (clientWriteGscScript(client, tab->path, text) == CLIENT_OK) tab->dirty = false;
    free(text);
}

static int onAutosaveTick(void *data) {
    (void)data;

    for (size_t i = 0; i < tabCount; i++) {
        if (openTabs[i].dirty) saveTab(&openTabs[i]);
    }
    return 1;
}

static void onNotify(uiScintilla *editor, void *notification, void *data) {
    (void)data;
    const SCNotification *event = (const SCNotification *)notification;

    if (event->nmhdr.code == SCN_STYLENEEDED) {
        gscLexerStyle(editor);
        return;
    }

    if (event->nmhdr.code == SCN_MODIFIED &&
        (event->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT))) {
        GscTab *tab = tabOf(editor);
        if (tab) tab->dirty = true;
    }
}

static void configureEditor(uiScintilla *editor) {
    uiScintillaSend(editor, SCI_STYLESETFONT, STYLE_DEFAULT, (intptr_t)GSC_EDITOR_FONT);
    uiScintillaSend(editor, SCI_STYLESETSIZE, STYLE_DEFAULT, GSC_EDITOR_FONT_SIZE);
    uiScintillaSend(editor, SCI_STYLECLEARALL, 0, 0);

    intptr_t marginWidth = uiScintillaSend(editor, SCI_TEXTWIDTH, STYLE_LINENUMBER,
                                           (intptr_t)"_9999");
    uiScintillaSend(editor, SCI_SETMARGINTYPEN, 0, SC_MARGIN_NUMBER);
    uiScintillaSend(editor, SCI_SETMARGINWIDTHN, 0, marginWidth);

    uiScintillaSend(editor, SCI_SETTABWIDTH, GSC_EDITOR_TAB_WIDTH, 0);
    uiScintillaSend(editor, SCI_SETEOLMODE, SC_EOL_CRLF, 0);
    uiScintillaSend(editor, SCI_SETSCROLLWIDTHTRACKING, 1, 0);

    gscLexerConfigure(editor);
}

static void closeTabAt(size_t index) {
    GscTab *tab = &openTabs[index];

    if (tab->dirty) saveTab(tab);

    uiTabDelete(tabs, (int)index);
    uiControlDestroy(uiControl(tab->editor));

    memmove(&openTabs[index], &openTabs[index + 1],
            (tabCount - index - 1) * sizeof(GscTab));
    tabCount--;

    if (tabCount > 0) uiTabSetSelected(tabs, (int)(index < tabCount ? index : tabCount - 1));
}

static void onClosing(uiTab *sender, int index, void *data) {
    (void)sender; (void)data;

    if (index >= 0 && (size_t)index < tabCount) closeTabAt((size_t)index);
}

uiControl *gscEditorCreate(Client *clientInstance, uiWindow *window) {
    client = clientInstance;
    host = window;

    tabs = uiNewTab();
    uiTabOnClosing(tabs, onClosing, NULL);
    uiTimer(GSC_AUTOSAVE_INTERVAL, onAutosaveTick, NULL);

    return uiControl(tabs);
}

void gscEditorOpen(const char *path) {
    for (size_t i = 0; i < tabCount; i++) {
        if (strcmp(openTabs[i].path, path) == 0) {
            uiTabSetSelected(tabs, (int)i);
            return;
        }
    }

    if (tabCount >= GSC_TAB_MAX) {
        uiMsgBoxError(host, "GSC Editor", "Too many files open, close a tab first.");
        return;
    }

    char *content = NULL;
    if (clientReadGscScript(client, path, &content) != CLIENT_OK) {
        uiMsgBoxError(host, "GSC Editor", "Could not read the script.");
        return;
    }

    GscTab *tab = &openTabs[tabCount];
    snprintf(tab->path, sizeof(tab->path), "%s", path);
    tab->editor = uiNewScintilla();
    tab->dirty = false;

    configureEditor(tab->editor);
    uiScintillaSend(tab->editor, SCI_SETTEXT, 0, (intptr_t)content);
    uiScintillaSend(tab->editor, SCI_EMPTYUNDOBUFFER, 0, 0);
    free(content);

    uiScintillaOnNotify(tab->editor, onNotify, NULL);
    uiTabAppend(tabs, tab->path, uiControl(tab->editor));

    tabCount++;
    uiTabSetSelected(tabs, (int)tabCount - 1);
}

static bool isInside(const char *path, const char *folder) {
    size_t length = strlen(folder);
    return strncmp(path, folder, length) == 0 && path[length] == '/';
}

void gscEditorClose(const char *path) {
    for (size_t i = tabCount; i > 0; i--) {
        const char *open = openTabs[i - 1].path;
        if (strcmp(open, path) == 0 || isInside(open, path)) closeTabAt(i - 1);
    }
}

void gscEditorFlush(void) {
    for (size_t i = 0; i < tabCount; i++) {
        if (openTabs[i].dirty) saveTab(&openTabs[i]);
    }
}
