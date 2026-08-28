#include "gui/gsc.h"

#include <windows.h>
#include <shellapi.h>
#include <ui.h>
#include <ui_windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gui.h"
#include "client/gsc.h"
#include "gui/gsc/editor.h"
#include "gui/gsc/help.h"
#include "resource_ids.h"
#include "win/resources.h"

#define GSC_WINDOW_TITLE "GSC Mods"
#define GSC_WINDOW_WIDTH 1080
#define GSC_WINDOW_HEIGHT 680

#define GSC_TREE_TITLE "Mods"

#define GSC_HINT_IDLE "Changes are saved as you type and apply the next time a map loads."
#define GSC_HINT_ONGOING "A game is in progress: restart the map to apply your changes."

#define GSC_NODE_MAX 512
#define GSC_NODE_PATH_SIZE 192
#define GSC_MOD_ENTRY "main.gsc"

typedef enum {
    GSC_ICON_MOD,
    GSC_ICON_FOLDER,
    GSC_ICON_FILE
} GscIcon;

#define GSC_ICON_COUNT 3

static const int GSC_ICON_RESOURCE[GSC_ICON_COUNT] = {
    IDR_PNG_MOD,
    IDR_PNG_FOLDER,
    IDR_PNG_FILE,
};

typedef struct {
    char path[GSC_NODE_PATH_SIZE];
    GscIcon icon;
    uiTreeItem *item;
} GscNode;

typedef struct {
    const char *name;
    GscEditAction action;
    int key;
} GscEditItem;

static const GscEditItem GSC_EDIT_ITEM[] = {
    {"Undo", GSC_EDIT_UNDO, 'Z'},
    {"Redo", GSC_EDIT_REDO, 'Y'},
    {NULL, GSC_EDIT_UNDO, 0},
    {"Cut", GSC_EDIT_CUT, 'X'},
    {"Copy", GSC_EDIT_COPY, 'C'},
    {"Paste", GSC_EDIT_PASTE, 'V'},
    {NULL, GSC_EDIT_UNDO, 0},
    {"Select All", GSC_EDIT_SELECT_ALL, 'A'},
};

#define GSC_EDIT_ITEM_COUNT (sizeof(GSC_EDIT_ITEM) / sizeof(*GSC_EDIT_ITEM))

static Client *client = NULL;
static uiWindow *gscWindow = NULL;
static uiTree *modTree = NULL;
static ClientGscMods mods = {};

static GscNode nodes[GSC_NODE_MAX];
static size_t nodeCount = 0;
static int treeIcons[GSC_ICON_COUNT];

static uiMenuItem *newFileItem = NULL;
static uiMenuItem *newFolderItem = NULL;
static uiMenuItem *deleteItem = NULL;
static uiLabel *hintLabel = NULL;

typedef struct {
    uiWindow *win;
    uiEntry *entry;
    char *result;
    int done;
} TextPrompt;

static void textPromptOk(uiButton *b, void *data) {
    (void)b;
    TextPrompt *p = (TextPrompt *)data;
    char *t = uiEntryText(p->entry);
    p->result = _strdup(t ? t : "");
    if (t) uiFreeText(t);
    p->done = 1;
}

static void textPromptCancel(uiButton *b, void *data) {
    (void)b;
    ((TextPrompt *)data)->done = 1;
}

static int textPromptClosing(uiWindow *w, void *data) {
    (void)w;
    ((TextPrompt *)data)->done = 1;
    return 0;
}

static char *promptText(const char *title, const char *label) {
    TextPrompt p;
    memset(&p, 0, sizeof(p));

    p.win = uiNewWindow(title, 340, 120, 0);
    uiWindowSetMargined(p.win, 1);
    uiWindowOnClosing(p.win, textPromptClosing, &p);

    uiBox *box = uiNewVerticalBox();
    uiBoxSetPadded(box, 1);
    uiBoxAppend(box, uiControl(uiNewLabel(label)), 0);

    p.entry = uiNewEntry();
    uiBoxAppend(box, uiControl(p.entry), 0);

    uiBox *row = uiNewHorizontalBox();
    uiBoxSetPadded(row, 1);
    uiButton *ok = uiNewButton("OK");
    uiButton *cancel = uiNewButton("Cancel");
    uiButtonOnClicked(ok, textPromptOk, &p);
    uiButtonOnClicked(cancel, textPromptCancel, &p);
    uiBoxAppend(row, uiControl(ok), 1);
    uiBoxAppend(row, uiControl(cancel), 1);
    uiBoxAppend(box, uiControl(row), 0);

    uiWindowSetChild(p.win, uiControl(box));

    if (gscWindow) uiControlDisable(uiControl(gscWindow));
    uiControlShow(uiControl(p.win));
    while (!p.done) uiMainStep(1);
    if (gscWindow) uiControlEnable(uiControl(gscWindow));
    uiControlDestroy(uiControl(p.win));
    return p.result;
}

static GscNode *selectedNode(void) {
    uiTreeItem *item = uiTreeSelected(modTree);
    return item ? (GscNode *)uiTreeItemData(item) : NULL;
}

static bool nodeIsFolder(const GscNode *node) {
    return node->icon != GSC_ICON_FILE;
}

static void loadTreeIcons(void) {
    for (int icon = 0; icon < GSC_ICON_COUNT; icon++) {
        void *data = NULL;
        uint32_t size = 0;

        treeIcons[icon] = -1;
        if (!resourcesGetData(GSC_ICON_RESOURCE[icon], &data, &size)) continue;

        uiImage *image = uiNewImageFromData(data, size);
        if (!image) continue;

        treeIcons[icon] = uiTreeAppendIcon(modTree, image);
        uiFreeImage(image);
    }
}

static const char *nodeFolder(const GscNode *node) {
    if (!node) return NULL;
    if (nodeIsFolder(node)) return node->path;

    const char *slash = strrchr(node->path, '/');
    if (!slash) return node->path;

    static char folder[GSC_NODE_PATH_SIZE];
    snprintf(folder, sizeof(folder), "%.*s", (int)(slash - node->path), node->path);
    return folder;
}

static void updateHint(void) {
    const GuiSnapshot *snapshot = guiGetSnapshot();
    bool ongoing = snapshot->stateValid && snapshot->state.isZombiesGameOngoing;
    uiLabelSetText(hintLabel, ongoing ? GSC_HINT_ONGOING : GSC_HINT_IDLE);
}

static void updateMenuItems(void) {
    if (selectedNode()) {
        uiMenuItemEnable(newFileItem);
        uiMenuItemEnable(newFolderItem);
        uiMenuItemEnable(deleteItem);
    } else {
        uiMenuItemDisable(newFileItem);
        uiMenuItemDisable(newFolderItem);
        uiMenuItemDisable(deleteItem);
    }
}

static GscNode *findNode(const char *path) {
    for (size_t i = 0; i < nodeCount; i++) {
        if (strcmp(nodes[i].path, path) == 0) return &nodes[i];
    }
    return NULL;
}

static GscNode *addNode(uiTreeItem *parent, const char *path, const char *text, GscIcon icon) {
    if (nodeCount >= GSC_NODE_MAX) return NULL;
    if (strlen(path) >= GSC_NODE_PATH_SIZE) return NULL;

    uiTreeItem *item = uiTreeAppend(modTree, parent, text);
    if (!item) return NULL;

    GscNode *node = &nodes[nodeCount++];
    snprintf(node->path, sizeof(node->path), "%s", path);
    node->icon = icon;
    node->item = item;
    uiTreeItemSetData(item, node);
    if (treeIcons[icon] >= 0) uiTreeItemSetIcon(item, treeIcons[icon]);
    return node;
}

static void addModEntry(const GscNode *mod, const char *relative, bool folder) {
    char path[GSC_NODE_PATH_SIZE];
    uiTreeItem *parent = mod->item;

    snprintf(path, sizeof(path), "%s", mod->path);

    for (const char *segment = relative; *segment;) {
        const char *slash = strchr(segment, '/');
        size_t length = slash ? (size_t)(slash - segment) : strlen(segment);
        size_t used = strlen(path);
        if (length == 0 || used + 1 + length >= sizeof(path)) return;

        path[used] = '/';
        memcpy(path + used + 1, segment, length);
        path[used + 1 + length] = '\0';

        bool leafFile = slash == NULL && !folder;

        GscNode *node = findNode(path);
        if (!node)
            node = addNode(parent, path, path + used + 1,
                           leafFile ? GSC_ICON_FILE : GSC_ICON_FOLDER);
        if (!node) return;

        parent = node->item;
        segment += slash ? length + 1 : length;
    }
}

static void reloadMods(void) {
    clientFreeGscMods(&mods);
    if (clientGetGscMods(client, &mods) != CLIENT_OK) {
        clientFreeGscMods(&mods);
    }

    uiTreeClear(modTree);
    nodeCount = 0;

    for (size_t i = 0; i < mods.modCount; i++) {
        const ClientGscMod *mod = &mods.mods[i];
        GscNode *node = addNode(NULL, mod->name, mod->name, GSC_ICON_MOD);
        if (!node) break;

        for (size_t e = 0; e < mod->entryCount; e++) {
            addModEntry(node, mod->entries[e].path, mod->entries[e].folder);
        }
    }

    uiTreeExpandAll(modTree);
    updateMenuItems();
    updateHint();
}

static void onSelectionChanged(uiTree *tree, void *data) {
    (void)tree; (void)data;

    updateMenuItems();

    const GscNode *node = selectedNode();
    if (node && !nodeIsFolder(node)) gscEditorOpen(node->path);
}

static void onNewModClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)item; (void)window; (void)data;

    char *name = promptText("New Mod", "Mod name:");
    if (!name) return;
    if (name[0] == '\0') {
        uiMsgBoxError(gscWindow, "New mod", "Enter a name for the mod.");
        free(name);
        return;
    }

    if (clientCreateGscMod(client, name) != CLIENT_OK) {
        uiMsgBoxError(gscWindow, "New mod", "Could not create the mod.");
        free(name);
        return;
    }

    char relative[GSC_NODE_PATH_SIZE];
    int n = snprintf(relative, sizeof(relative), "%s/" GSC_MOD_ENTRY, name);
    free(name);

    reloadMods();

    if (n > 0 && (size_t)n < sizeof(relative)) gscEditorOpen(relative);
}

static void createFile(const GscNode *node) {
    const char *folder = nodeFolder(node);
    if (!folder) return;

    char label[GSC_NODE_PATH_SIZE + 32];
    snprintf(label, sizeof(label), "File name, relative to %s:", folder);

    char *name = promptText("New File", label);
    if (!name) return;

    char relative[GSC_NODE_PATH_SIZE];
    int n = snprintf(relative, sizeof(relative), "%s/%s", folder, name);
    free(name);
    if (n <= 0 || (size_t)n >= sizeof(relative)) return;

    if (clientCreateGscScript(client, relative) != CLIENT_OK) {
        uiMsgBoxError(gscWindow, "New file", "Could not create the script. Names must end in .gsc.");
        return;
    }

    reloadMods();
    gscEditorOpen(relative);
}

static void createFolder(const GscNode *node) {
    const char *folder = nodeFolder(node);
    if (!folder) return;

    char label[GSC_NODE_PATH_SIZE + 32];
    snprintf(label, sizeof(label), "Folder name, relative to %s:", folder);

    char *name = promptText("New Folder", label);
    if (!name) return;

    char relative[GSC_NODE_PATH_SIZE];
    int n = snprintf(relative, sizeof(relative), "%s/%s", folder, name);
    free(name);
    if (n <= 0 || (size_t)n >= sizeof(relative)) return;

    if (clientCreateGscFolder(client, relative) != CLIENT_OK) {
        uiMsgBoxError(gscWindow, "New folder", "Could not create the folder.");
        return;
    }

    reloadMods();
}

static void deleteNode(const GscNode *node) {
    if (!node) return;

    char message[GSC_NODE_PATH_SIZE + 64];
    snprintf(message, sizeof(message),
             nodeIsFolder(node) ? "Delete \"%s\" and everything inside it?" : "Delete \"%s\"?",
             node->path);
    if (uiMsgBoxOkCancel(gscWindow, "Delete", message) != 1) return;

    gscEditorClose(node->path);

    if (clientDeleteGscPath(client, node->path) != CLIENT_OK) {
        uiMsgBoxError(gscWindow, "Delete", clientLastErrorMessage(client));
    }
    reloadMods();
}

static void revealNode(const GscNode *node) {
    if (!node || mods.folder[0] == '\0') return;

    char path[MAX_PATH];
    int n = snprintf(path, sizeof(path), "%s\\%s", mods.folder, node->path);
    if (n <= 0 || (size_t)n >= sizeof(path)) return;

    for (char *c = path; *c; c++) {
        if (*c == '/') *c = '\\';
    }

    char select[MAX_PATH + 16];
    snprintf(select, sizeof(select), "/select,\"%s\"", path);
    ShellExecuteA(NULL, "open", "explorer.exe", select, NULL, SW_SHOWNORMAL);
}

static void onNewFileClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)item; (void)window; (void)data;
    createFile(selectedNode());
}

static void onNewFolderClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)item; (void)window; (void)data;
    createFolder(selectedNode());
}

static void onDeleteClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)item; (void)window; (void)data;
    deleteNode(selectedNode());
}

static void onSaveClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)item; (void)window; (void)data;
    gscEditorFlush();
}

static void onOpenFolderClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)item; (void)window; (void)data;
    if (mods.folder[0] == '\0') return;
    ShellExecuteA(NULL, "open", mods.folder, NULL, NULL, SW_SHOWNORMAL);
}

static void onEditClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)item; (void)window;
    gscEditorEdit((GscEditAction)(intptr_t)data);
}

static void onRefreshClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)item; (void)window; (void)data;
    reloadMods();
}

static void onExpandAllClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)item; (void)window; (void)data;
    uiTreeExpandAll(modTree);
}

static void onCollapseAllClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)item; (void)window; (void)data;
    uiTreeCollapseAll(modTree);
}

static void onWordWrapClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)window; (void)data;
    gscEditorSetWordWrap(uiMenuItemChecked(item) != 0);
}

static void onLineNumbersClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)window; (void)data;
    gscEditorSetLineNumbers(uiMenuItemChecked(item) != 0);
}

static void onHelpClicked(uiMenuItem *item, uiWindow *window, void *data) {
    (void)item; (void)data;
    uiGscHelpShow(window);
}

typedef struct {
    const char *name;
    void (*action)(const GscNode *);
} GscContextItem;

static const GscContextItem GSC_CONTEXT_ITEM[] = {
    {"New File...", createFile},
    {"New Folder...", createFolder},
    {NULL, NULL},
    {"Delete", deleteNode},
    {NULL, NULL},
    {"Reveal in Explorer", revealNode},
};

#define GSC_CONTEXT_ITEM_COUNT (sizeof(GSC_CONTEXT_ITEM) / sizeof(*GSC_CONTEXT_ITEM))
#define GSC_CONTEXT_ID_FIRST 1

static void onTreeContextMenu(uiTree *tree, uiTreeItem *item, void *data) {
    (void)tree; (void)data;

    const GscNode *node = (const GscNode *)uiTreeItemData(item);
    if (!node) return;

    POINT cursor;
    if (!GetCursorPos(&cursor)) return;

    HMENU menu = CreatePopupMenu();
    if (!menu) return;

    for (size_t i = 0; i < GSC_CONTEXT_ITEM_COUNT; i++) {
        if (GSC_CONTEXT_ITEM[i].name == NULL) {
            AppendMenuA(menu, MF_SEPARATOR, 0, NULL);
            continue;
        }
        AppendMenuA(menu, MF_STRING, GSC_CONTEXT_ID_FIRST + i, GSC_CONTEXT_ITEM[i].name);
    }

    int id = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
                            cursor.x, cursor.y, 0,
                            (HWND)uiControlHandle(uiControl(gscWindow)), NULL);
    DestroyMenu(menu);

    size_t index = (size_t)(id - GSC_CONTEXT_ID_FIRST);
    if (id >= GSC_CONTEXT_ID_FIRST && index < GSC_CONTEXT_ITEM_COUNT)
        GSC_CONTEXT_ITEM[index].action(node);
}

static uiMenuItem *appendItem(uiMenu *menu, const char *name,
                              void (*onClicked)(uiMenuItem *, uiWindow *, void *)) {
    uiMenuItem *item = uiMenuAppendItem(menu, name);
    uiMenuItemOnClicked(item, onClicked, NULL);
    return item;
}

static void buildFileMenu(uiMenuBar *bar) {
    uiMenu *menu = uiMenuBarAppendMenu(bar, "File");

    uiMenuItem *newMod = appendItem(menu, "New Mod...", onNewModClicked);
    uiMenuItemSetShortcut(newMod, uiModifierCtrl, 'M');

    newFileItem = appendItem(menu, "New File...", onNewFileClicked);
    uiMenuItemSetShortcut(newFileItem, uiModifierCtrl, 'N');

    newFolderItem = appendItem(menu, "New Folder...", onNewFolderClicked);
    uiMenuItemSetShortcut(newFolderItem, uiModifierCtrl | uiModifierShift, 'N');

    deleteItem = appendItem(menu, "Delete", onDeleteClicked);

    uiMenuAppendSeparator(menu);
    uiMenuItem *save = appendItem(menu, "Save", onSaveClicked);
    uiMenuItemSetShortcut(save, uiModifierCtrl, 'S');

    uiMenuAppendSeparator(menu);
    uiMenuItem *openFolder = appendItem(menu, "Open Mods Folder", onOpenFolderClicked);
    uiMenuItemSetShortcut(openFolder, uiModifierCtrl | uiModifierShift, 'O');
}

static void buildEditMenu(uiMenuBar *bar) {
    uiMenu *menu = uiMenuBarAppendMenu(bar, "Edit");

    for (size_t i = 0; i < GSC_EDIT_ITEM_COUNT; i++) {
        if (GSC_EDIT_ITEM[i].name == NULL) {
            uiMenuAppendSeparator(menu);
            continue;
        }

        uiMenuItem *item = uiMenuAppendItem(menu, GSC_EDIT_ITEM[i].name);
        uiMenuItemOnClicked(item, onEditClicked, (void *)(intptr_t)GSC_EDIT_ITEM[i].action);
        uiMenuItemSetShortcut(item, uiModifierCtrl, GSC_EDIT_ITEM[i].key);
    }
}

static void buildViewMenu(uiMenuBar *bar) {
    uiMenu *menu = uiMenuBarAppendMenu(bar, "View");

    uiMenuItem *refresh = appendItem(menu, "Refresh", onRefreshClicked);
    uiMenuItemSetShortcut(refresh, 0, uiExtKeyF5);

    uiMenuAppendSeparator(menu);
    appendItem(menu, "Expand All Mods", onExpandAllClicked);
    appendItem(menu, "Collapse All Mods", onCollapseAllClicked);

    uiMenuAppendSeparator(menu);
    uiMenuItem *wordWrap = uiMenuAppendCheckItem(menu, "Word Wrap");
    uiMenuItemOnClicked(wordWrap, onWordWrapClicked, NULL);
    uiMenuItemSetShortcut(wordWrap, uiModifierAlt, 'Z');

    uiMenuItem *lineNumbers = uiMenuAppendCheckItem(menu, "Line Numbers");
    uiMenuItemOnClicked(lineNumbers, onLineNumbersClicked, NULL);
    uiMenuItemSetChecked(lineNumbers, 1);
}

static void buildHelpMenu(uiMenuBar *bar) {
    uiMenu *menu = uiMenuBarAppendMenu(bar, "Help");

    uiMenuItem *help = appendItem(menu, "GSC Mods Help", onHelpClicked);
    uiMenuItemSetShortcut(help, 0, uiExtKeyF1);
}

static uiMenuBar *buildMenuBar(void) {
    uiMenuBar *bar = uiNewMenuBar();

    buildFileMenu(bar);
    buildEditMenu(bar);
    buildViewMenu(bar);
    buildHelpMenu(bar);

    return bar;
}

static uiControl *buildSidebar(void) {
    uiBox *box = uiNewVerticalBox();
    uiBoxSetPadded(box, 1);
    uiBoxAppend(box, uiControl(uiNewLabel(GSC_TREE_TITLE)), 0);

    modTree = uiNewTree();
    uiTreeOnSelectionChanged(modTree, onSelectionChanged, NULL);
    uiTreeOnItemContextMenu(modTree, onTreeContextMenu, NULL);
    loadTreeIcons();
    uiBoxAppend(box, uiControl(modTree), 1);

    return uiControl(box);
}

static uiControl *buildContent(void) {
    uiBox *columns = uiNewHorizontalBox();
    uiBoxSetPadded(columns, 1);
    uiBoxAppend(columns, buildSidebar(), 0);
    uiBoxAppend(columns, gscEditorCreate(client, gscWindow), 1);

    uiBox *box = uiNewVerticalBox();
    uiBoxSetPadded(box, 1);
    uiBoxAppend(box, uiControl(columns), 1);

    hintLabel = uiNewLabel(GSC_HINT_IDLE);
    uiBoxAppend(box, uiControl(hintLabel), 0);

    return uiControl(box);
}

static int onWindowClose(uiWindow *window, void *data) {
    (void)data;
    gscEditorFlush();
    uiControlHide(uiControl(window));
    return 0;
}

static void buildGscWindow(void) {
    gscWindow = uiNewWindow(GSC_WINDOW_TITLE, GSC_WINDOW_WIDTH, GSC_WINDOW_HEIGHT, 0);
    uiWindowOnClosing(gscWindow, onWindowClose, NULL);
    uiWindowSetMargined(gscWindow, 1);
    uiWindowSetChild(gscWindow, buildContent());
    uiWindowSetMenuBar(gscWindow, buildMenuBar());
    uiWindowSetIcon(gscWindow, IDI_ICON1);
}

void uiGscShow(Client *clientInstance) {
    client = clientInstance;

    if (gscWindow == NULL) buildGscWindow();

    reloadMods();

    uiControlShow(uiControl(gscWindow));
    SetForegroundWindow((HWND)uiControlHandle(uiControl(gscWindow)));
}
