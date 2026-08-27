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

static Client *client = NULL;
static uiWindow *gscWindow = NULL;
static uiTree *modTree = NULL;
static ClientGscMods mods = {};

static GscNode nodes[GSC_NODE_MAX];
static size_t nodeCount = 0;
static int treeIcons[GSC_ICON_COUNT];

static uiButton *newFileButton = NULL;
static uiButton *removeButton = NULL;
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

static const char *selectedMod(void) {
    const GscNode *node = selectedNode();
    if (!node) return NULL;

    const char *slash = strchr(node->path, '/');
    if (!slash) return node->path;

    static char name[GSC_NODE_PATH_SIZE];
    snprintf(name, sizeof(name), "%.*s", (int)(slash - node->path), node->path);
    return name;
}

static void updateHint(void) {
    const GuiSnapshot *snapshot = guiGetSnapshot();
    bool ongoing = snapshot->stateValid && snapshot->state.isZombiesGameOngoing;
    uiLabelSetText(hintLabel, ongoing ? GSC_HINT_ONGOING : GSC_HINT_IDLE);
}

static void updateButtons(void) {
    if (selectedNode()) {
        uiControlEnable(uiControl(newFileButton));
        uiControlEnable(uiControl(removeButton));
    } else {
        uiControlDisable(uiControl(newFileButton));
        uiControlDisable(uiControl(removeButton));
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

static void addModFile(const GscNode *mod, const char *relative) {
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

        GscNode *node = findNode(path);
        if (!node)
            node = addNode(parent, path, path + used + 1,
                           slash != NULL ? GSC_ICON_FOLDER : GSC_ICON_FILE);
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

        for (size_t f = 0; f < mod->fileCount; f++) {
            addModFile(node, mod->files[f].path);
        }
    }

    uiTreeExpandAll(modTree);
    updateButtons();
    updateHint();
}

static void onSelectionChanged(uiTree *tree, void *data) {
    (void)tree; (void)data;

    updateButtons();

    const GscNode *node = selectedNode();
    if (node && !nodeIsFolder(node)) gscEditorOpen(node->path);
}

static void onNewModClicked(uiButton *button, void *data) {
    (void)button; (void)data;

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

static void onNewFileClicked(uiButton *button, void *data) {
    (void)button; (void)data;

    const char *mod = selectedMod();
    if (!mod) return;

    char label[GSC_NODE_PATH_SIZE + 32];
    snprintf(label, sizeof(label), "File name, relative to %s:", mod);

    char *name = promptText("New File", label);
    if (!name) return;

    char relative[GSC_NODE_PATH_SIZE];
    int n = snprintf(relative, sizeof(relative), "%s/%s", mod, name);
    free(name);
    if (n <= 0 || (size_t)n >= sizeof(relative)) return;

    if (clientCreateGscScript(client, relative) != CLIENT_OK) {
        uiMsgBoxError(gscWindow, "New file", "Could not create the script. Names must end in .gsc.");
        return;
    }

    reloadMods();
    gscEditorOpen(relative);
}

static void onRemoveClicked(uiButton *button, void *data) {
    (void)button; (void)data;

    GscNode *node = selectedNode();
    if (!node) return;

    char message[GSC_NODE_PATH_SIZE + 64];
    snprintf(message, sizeof(message),
             nodeIsFolder(node) ? "Delete \"%s\" and everything inside it?" : "Delete \"%s\"?",
             node->path);
    if (uiMsgBoxOkCancel(gscWindow, "Remove", message) != 1) return;

    gscEditorClose(node->path);

    if (clientDeleteGscPath(client, node->path) != CLIENT_OK) {
        uiMsgBoxError(gscWindow, "Remove", clientLastErrorMessage(client));
    }
    reloadMods();
}

static void onOpenFolderClicked(uiButton *button, void *data) {
    (void)button; (void)data;
    if (mods.folder[0] == '\0') return;
    ShellExecuteA(NULL, "open", mods.folder, NULL, NULL, SW_SHOWNORMAL);
}

static void onRefreshClicked(uiButton *button, void *data) {
    (void)button; (void)data;
    reloadMods();
}

static uiControl *buildSidebar(void) {
    uiBox *box = uiNewVerticalBox();
    uiBoxSetPadded(box, 1);
    uiBoxAppend(box, uiControl(uiNewLabel(GSC_TREE_TITLE)), 0);

    modTree = uiNewTree();
    uiTreeOnSelectionChanged(modTree, onSelectionChanged, NULL);
    loadTreeIcons();
    uiBoxAppend(box, uiControl(modTree), 1);

    uiButton *newModButton = uiNewButton("New Mod...");
    newFileButton = uiNewButton("New File...");
    removeButton = uiNewButton("Remove");
    uiButton *openFolderButton = uiNewButton("Open Folder");
    uiButton *refreshButton = uiNewButton("Refresh");

    uiButtonOnClicked(newModButton, onNewModClicked, NULL);
    uiButtonOnClicked(newFileButton, onNewFileClicked, NULL);
    uiButtonOnClicked(removeButton, onRemoveClicked, NULL);
    uiButtonOnClicked(openFolderButton, onOpenFolderClicked, NULL);
    uiButtonOnClicked(refreshButton, onRefreshClicked, NULL);

    uiBox *creation = uiNewHorizontalBox();
    uiBoxSetPadded(creation, 1);
    uiBoxAppend(creation, uiControl(newModButton), 1);
    uiBoxAppend(creation, uiControl(newFileButton), 1);
    uiBoxAppend(box, uiControl(creation), 0);

    uiBox *maintenance = uiNewHorizontalBox();
    uiBoxSetPadded(maintenance, 1);
    uiBoxAppend(maintenance, uiControl(removeButton), 1);
    uiBoxAppend(maintenance, uiControl(refreshButton), 1);
    uiBoxAppend(box, uiControl(maintenance), 0);

    uiBoxAppend(box, uiControl(openFolderButton), 0);

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
    uiWindowSetIcon(gscWindow, IDI_ICON1);
}

void uiGscShow(Client *clientInstance) {
    client = clientInstance;

    if (gscWindow == NULL) buildGscWindow();

    reloadMods();

    uiControlShow(uiControl(gscWindow));
    SetForegroundWindow((HWND)uiControlHandle(uiControl(gscWindow)));
}
