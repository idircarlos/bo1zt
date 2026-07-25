#include "gui/camo.h"
#include "gui/camo/help.h"
#include "gui/camo/viewer.h"
#include "gui.h"
#include "logic/camo/manager.h"
#include "utils/iwi.h"
#include "logger.h"
#include "resource_ids.h"
#include <ui.h>
#include <windows.h>
#include <ui_windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "miniz.h"

#define CAMO_WINDOW_TITLE "Camo Manager"
#define CAMO_WINDOW_WIDTH 1180
#define CAMO_WINDOW_HEIGHT 720

static uiWindow *parent = NULL;
static uiWindow *camoWindow = NULL;

static uiGroup *viewerGroup = NULL;
static uiGLArea *viewerArea = NULL;

static CamoManager *manager = NULL;
static CamoViewer *viewer = NULL;

static void refreshViewer(void);

static const char *slotLabels[CAMO_FILE_TYPE_COUNT] = {
    "Spec",
    "Col",
    "Env",
    "Norm",
};

#define CAMO_MAX_EXTRA 99

static const char *camoTypeBaseLabel(CamoFileType type) {
    switch (type) {
    case CAMO_FILE_SPEC:   return "  Spec ~~- ";
    case CAMO_FILE_COLOR:  return "  Color ~- ";
    case CAMO_FILE_ENV:    return "  Env ~ ";
    case CAMO_FILE_NORMAL: return "  Normal  ";
    default:               return "  Unknown  ";
    }
}

static void camoFileLabel(char *out, size_t size, CamoFileType type,
                          unsigned int number) {
    if (!out || size == 0) return;
    const char *base = camoTypeBaseLabel(type);
    if (number == 0) {
        snprintf(out, size, "%s", base);
    } else {
        snprintf(out, size, "%s Extra %u", base, number);
    }
}

static bool camoHasIwiExtension(const char *path) {
    if (!path) return false;
    size_t len = strlen(path);
    return len >= 4 && _stricmp(path + len - 4, ".iwi") == 0;
}

#define CAMO_THUMB_SIZE 32
#define CAMO_EXTRA_SIZE 40

static unsigned char *scaleThumb(const unsigned char *src, int srcW, int srcH, int size) {
    if (!src || srcW <= 0 || srcH <= 0 || size <= 0) return NULL;
    unsigned char *dst = (unsigned char *)calloc((size_t)size * size, 4);
    if (!dst) return NULL;

    int dw = size, dh = size;
    if (srcW >= srcH) {
        dh = (int)((long long)size * srcH / srcW);
        if (dh < 1) dh = 1;
    } else {
        dw = (int)((long long)size * srcW / srcH);
        if (dw < 1) dw = 1;
    }
    int ox = (size - dw) / 2;
    int oy = (size - dh) / 2;

    for (int y = 0; y < dh; y++) {
        int sy0 = (int)((long long)y * srcH / dh);
        int sy1 = (int)((long long)(y + 1) * srcH / dh);
        if (sy1 <= sy0) sy1 = sy0 + 1;
        for (int x = 0; x < dw; x++) {
            int sx0 = (int)((long long)x * srcW / dw);
            int sx1 = (int)((long long)(x + 1) * srcW / dw);
            if (sx1 <= sx0) sx1 = sx0 + 1;
            unsigned long r = 0, g = 0, b = 0, a = 0, n = 0;
            for (int sy = sy0; sy < sy1 && sy < srcH; sy++) {
                for (int sx = sx0; sx < sx1 && sx < srcW; sx++) {
                    const unsigned char *s = src + ((size_t)sy * srcW + sx) * 4;
                    r += s[0]; g += s[1]; b += s[2]; a += s[3];
                    n++;
                }
            }
            if (n == 0) n = 1;
            unsigned char *d = dst + ((size_t)(y + oy) * size + (x + ox)) * 4;
            d[0] = (unsigned char)(r / n);
            d[1] = (unsigned char)(g / n);
            d[2] = (unsigned char)(b / n);
            d[3] = (unsigned char)(a / n);
        }
    }
    return dst;
}

static unsigned char *decodeThumb(const char *path, int size) {
    if (!path || path[0] == '\0') return NULL;
    IwiImage img;
    if (!iwiLoad(path, &img, NULL, 0)) return NULL;
    unsigned char *thumb = scaleThumb(img.pixels, img.width, img.height, size);
    iwiFree(&img);
    return thumb;
}

static uiImage *thumbImageFromIwi(const char *path, int size) {
    unsigned char *thumb = decodeThumb(path, size);
    if (!thumb) return NULL;
    for (int i = 0; i < size * size; i++) {
        unsigned char *p = thumb + (size_t)i * 4;
        unsigned a = p[3];
        p[0] = (unsigned char)(p[0] * a / 255);
        p[1] = (unsigned char)(p[1] * a / 255);
        p[2] = (unsigned char)(p[2] * a / 255);
    }
    uiImage *image = uiNewImage((double)size, (double)size);
    uiImageAppend(image, thumb, size, size, size * 4);
    free(thumb);
    return image;
}

static bool thumbSetImageViewRGBA(uiImageView *view, const unsigned char *rgba,
                                    int size) {
    size_t pngLen = 0;
    void *png = tdefl_write_image_to_png_file_in_memory(rgba, size, size, 4, &pngLen);
    if (!png) return false;
    int ok = uiImageViewSetFromData(view, png, pngLen);
    mz_free(png);
    return ok != 0;
}

static bool thumbSetImageView(uiImageView *view, const char *path, int size) {
    unsigned char *thumb = decodeThumb(path, size);
    if (!thumb) return false;
    bool ok = thumbSetImageViewRGBA(view, thumb, size);
    free(thumb);
    return ok;
}

static void thumbSetPlaceholder(uiImageView *view, int size) {
    unsigned char *px = (unsigned char *)malloc((size_t)size * size * 4);
    if (!px) return;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            bool border = (x == 0 || y == 0 || x == size - 1 || y == size - 1);
            unsigned char *p = px + ((size_t)y * size + x) * 4;
            unsigned char v = border ? 150 : 210;
            p[0] = v; p[1] = v; p[2] = v; p[3] = 255;
        }
    }
    thumbSetImageViewRGBA(view, px, size);
    free(px);
}

static void rebuildCamoList(void);
static void rebuildBundleList(void);
static void rebuildWeaponAssignment(void);
static void refreshCamoDetails(void);
static void refreshActiveStatus(void);
static void refreshAssignButton(void);
static void onFileReplaceClicked(uiButton *button, void *data);
static void onFileRemoveClicked(uiButton *button, void *data);

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

static char *promptText(const char *title, const char *label, const char *initial) {
    TextPrompt p;
    memset(&p, 0, sizeof(p));

    p.win = uiNewWindow(title, 340, 120, 0);
    uiWindowSetMargined(p.win, 1);
    uiWindowOnClosing(p.win, textPromptClosing, &p);

    uiBox *box = uiNewVerticalBox();
    uiBoxSetPadded(box, 1);
    uiBoxAppend(box, uiControl(uiNewLabel(label)), 0);

    p.entry = uiNewEntry();
    if (initial) uiEntrySetText(p.entry, initial);
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

    if (camoWindow) uiControlDisable(uiControl(camoWindow));
    uiControlShow(uiControl(p.win));
    while (!p.done) uiMainStep(1);
    if (camoWindow) uiControlEnable(uiControl(camoWindow));
    uiControlDestroy(uiControl(p.win));
    return p.result;
}

typedef struct {
    uiWindow *win;
    uiCombobox *combo;
    int result;
    int done;
} PickPrompt;

static void pickPromptOk(uiButton *b, void *data) {
    (void)b;
    PickPrompt *p = (PickPrompt *)data;
    p->result = uiComboboxSelected(p->combo);
    p->done = 1;
}

static void pickPromptCancel(uiButton *b, void *data) {
    (void)b;
    ((PickPrompt *)data)->done = 1;
}

static int pickPromptClosing(uiWindow *w, void *data) {
    (void)w;
    ((PickPrompt *)data)->done = 1;
    return 0;
}

static int promptPick(const char *title, const char *label, const char **items,
                      size_t count) {
    if (count == 0) return -1;

    PickPrompt p;
    memset(&p, 0, sizeof(p));
    p.result = -1;

    p.win = uiNewWindow(title, 340, 120, 0);
    uiWindowSetMargined(p.win, 1);
    uiWindowOnClosing(p.win, pickPromptClosing, &p);

    uiBox *box = uiNewVerticalBox();
    uiBoxSetPadded(box, 1);
    uiBoxAppend(box, uiControl(uiNewLabel(label)), 0);

    p.combo = uiNewCombobox();
    for (size_t i = 0; i < count; ++i) {
        uiComboboxAppend(p.combo, items[i] ? items[i] : "");
    }
    uiComboboxSetSelected(p.combo, 0);
    uiBoxAppend(box, uiControl(p.combo), 0);

    uiBox *row = uiNewHorizontalBox();
    uiBoxSetPadded(row, 1);
    uiButton *ok = uiNewButton("OK");
    uiButton *cancel = uiNewButton("Cancel");
    uiButtonOnClicked(ok, pickPromptOk, &p);
    uiButtonOnClicked(cancel, pickPromptCancel, &p);
    uiBoxAppend(row, uiControl(ok), 1);
    uiBoxAppend(row, uiControl(cancel), 1);
    uiBoxAppend(box, uiControl(row), 0);

    uiWindowSetChild(p.win, uiControl(box));

    if (camoWindow) uiControlDisable(uiControl(camoWindow));
    uiControlShow(uiControl(p.win));
    while (!p.done) uiMainStep(1);
    if (camoWindow) uiControlEnable(uiControl(camoWindow));
    uiControlDestroy(uiControl(p.win));
    return p.result;
}

static bool camoGameLocation(char *out, size_t size) {
    const GuiSnapshot *snapshot = guiGetSnapshot();
    if (!snapshot || !snapshot->gameConfigValid) return false;
    if (snapshot->gameConfig.location[0] == '\0') return false;
    int n = snprintf(out, size, "%s", snapshot->gameConfig.location);
    return n > 0 && (size_t)n < size;
}

static bool isActiveBundle(const char *bundleId) {
    const char *active = camoManagerGetActiveBundleId(manager);
    return active && bundleId && strcmp(active, bundleId) == 0;
}

static bool activeBundleUsesCamo(const char *camoId) {
    const char *activeId = camoManagerGetActiveBundleId(manager);
    if (!activeId || activeId[0] == '\0' || !camoId) return false;
    size_t total = 0;
    const CamoBundle *bundles = camoManagerGetBundles(manager, &total);
    for (size_t i = 0; i < total; ++i) {
        if (!bundles[i].id || strcmp(bundles[i].id, activeId) != 0) continue;
        for (size_t j = 0; j < bundles[i].entryCount; ++j) {
            if (bundles[i].entries[j].camoId &&
                strcmp(bundles[i].entries[j].camoId, camoId) == 0) {
                return true;
            }
        }
    }
    return false;
}

static void reconcileActiveBundleRebuild(const char *bundleId) {
    if (!isActiveBundle(bundleId)) return;

    char location[MAX_PATH];
    if (!camoGameLocation(location, sizeof(location))) {
        uiMsgBox(parent, "Active bundle changed",
                 "This bundle is installed, but the game location is not set, so the "
                 "installed files were not refreshed. Reinstall it after setting the "
                 "game location.");
        return;
    }

    if (camoManagerBundleInstall(manager, bundleId, location) != CAMO_RESULT_OK) {
        uiMsgBoxError(parent, "Active bundle changed",
                      "This bundle is installed, but refreshing the installed files "
                      "failed. Reinstall it from the Camo Bundles panel.");
    }
    refreshActiveStatus();
}

static uiTable *camoListTable = NULL;
static uiTableModel *camoListModel = NULL;
static uiTableModelHandler camoListHandler;
static size_t camoRowCount = 0;
static int selectedCamoRow = -1;

static uiImage **camoThumbs = NULL;
static size_t camoThumbCount = 0;
static uiImage *placeholderThumb = NULL;

static uiLabel *detailsTitle = NULL;

typedef struct {
    CamoFileType type;
    unsigned int number;
    char *source;
} EditorFile;

static EditorFile *editorFiles = NULL;
static size_t editorFileCount = 0;
static char *editingCamoId = NULL;

static uiBox *filesContainer = NULL;
static uiGrid *fileRowsGrid = NULL;

typedef struct {
    CamoFileType type;
    unsigned int number;
} FileRowKey;

static FileRowKey *fileKeys = NULL;
static size_t fileKeyCount = 0;

static int editorIndexOf(CamoFileType type, unsigned int number) {
    for (size_t i = 0; i < editorFileCount; ++i) {
        if (editorFiles[i].type == type && editorFiles[i].number == number) {
            return (int)i;
        }
    }
    return -1;
}

static void editorClearFiles(void) {
    for (size_t i = 0; i < editorFileCount; ++i) free(editorFiles[i].source);
    free(editorFiles);
    editorFiles = NULL;
    editorFileCount = 0;
}

static bool editorSetFile(CamoFileType type, unsigned int number, const char *source) {
    if (!source || source[0] == '\0') return false;
    char *copy = _strdup(source);
    if (!copy) {
        LOG_ERROR("Out of memory staging camo file");
        return false;
    }
    int idx = editorIndexOf(type, number);
    if (idx >= 0) {
        free(editorFiles[idx].source);
        editorFiles[idx].source = copy;
        return true;
    }
    EditorFile *grown =
        (EditorFile *)realloc(editorFiles, (editorFileCount + 1) * sizeof(EditorFile));
    if (!grown) {
        free(copy);
        LOG_ERROR("Out of memory growing camo file list");
        return false;
    }
    editorFiles = grown;
    editorFiles[editorFileCount].type = type;
    editorFiles[editorFileCount].number = number;
    editorFiles[editorFileCount].source = copy;
    editorFileCount += 1;
    return true;
}

static void editorRemoveFile(CamoFileType type, unsigned int number) {
    int idx = editorIndexOf(type, number);
    if (idx < 0) return;
    free(editorFiles[idx].source);
    for (size_t i = (size_t)idx; i + 1 < editorFileCount; ++i) {
        editorFiles[i] = editorFiles[i + 1];
    }
    editorFileCount -= 1;
}

static unsigned int editorNextExtraNumber(CamoFileType type) {
    unsigned int n = 1;
    while (n <= CAMO_MAX_EXTRA && editorIndexOf(type, n) >= 0) n++;
    return n;
}

static const Camo *selectedCamo(void) {
    if (selectedCamoRow < 0) return NULL;
    size_t total = 0;
    const Camo *camos = camoManagerGetCamos(manager, &total);
    if ((size_t)selectedCamoRow >= total) return NULL;
    return &camos[selectedCamoRow];
}

static void editorLoadFromCamo(const Camo *camo) {
    editorClearFiles();
    free(editingCamoId);
    editingCamoId = NULL;
    if (!camo) return;

    editingCamoId = _strdup(camo->id);
    for (size_t i = 0; i < camo->fileCount; ++i) {
        char managed[MAX_PATH];
        if (camoManagerCamoFilePath(manager, camo->id, camo->files[i].type,
                                    camo->files[i].number, managed, sizeof(managed))) {
            editorSetFile(camo->files[i].type, camo->files[i].number, managed);
        }
    }
}

static bool applyCamoFiles(void) {
    if (!editingCamoId) return false;

    const char *name = NULL;
    size_t total = 0;
    const Camo *camos = camoManagerGetCamos(manager, &total);
    for (size_t i = 0; i < total; ++i) {
        if (camos[i].id && strcmp(camos[i].id, editingCamoId) == 0) {
            name = camos[i].name;
            break;
        }
    }
    char *nameCopy = _strdup(name ? name : "");
    char *idCopy = _strdup(editingCamoId);
    if (!nameCopy || !idCopy) {
        free(nameCopy);
        free(idCopy);
        return false;
    }

    CamoFile *files = NULL;
    if (editorFileCount > 0) {
        files = (CamoFile *)calloc(editorFileCount, sizeof(CamoFile));
        if (!files) {
            free(nameCopy);
            free(idCopy);
            uiMsgBoxError(parent, "Camo files", "Out of memory.");
            return false;
        }
        for (size_t i = 0; i < editorFileCount; ++i) {
            files[i].type = editorFiles[i].type;
            files[i].number = editorFiles[i].number;
            files[i].fileName = editorFiles[i].source;
        }
    }

    CamoResult result = camoManagerCamoUpdate(manager, idCopy, nameCopy, files,
                                              editorFileCount);
    free(files);
    free(nameCopy);

    if (result != CAMO_RESULT_OK) {
        uiMsgBoxError(parent, "Camo files",
                      "Could not save the camo files. Verify the selected .iwi files "
                      "still exist and try again.");
        free(idCopy);
        return false;
    }

    bool rebuild = activeBundleUsesCamo(idCopy);
    const Camo *updated = NULL;
    camos = camoManagerGetCamos(manager, &total);
    for (size_t i = 0; i < total; ++i) {
        if (camos[i].id && strcmp(camos[i].id, idCopy) == 0) {
            updated = &camos[i];
            break;
        }
    }
    editorLoadFromCamo(updated);

    if (rebuild) {
        const char *activeId = camoManagerGetActiveBundleId(manager);
        if (activeId) reconcileActiveBundleRebuild(activeId);
    }

    free(idCopy);
    rebuildCamoList();
    refreshCamoDetails();
    return true;
}

static bool camoThumbPath(char *out, size_t size, const Camo *camo) {
    if (!camo || camo->fileCount == 0) return false;
    for (size_t i = 0; i < camo->fileCount; ++i) {
        if (camo->files[i].type == CAMO_FILE_COLOR && camo->files[i].number == 0) {
            return camoManagerCamoFilePath(manager, camo->id, camo->files[i].type,
                                           camo->files[i].number, out, size);
        }
    }
    return camoManagerCamoFilePath(manager, camo->id, camo->files[0].type,
                                   camo->files[0].number, out, size);
}

static uiImage *getPlaceholderThumb(void) {
    if (placeholderThumb) return placeholderThumb;
    int size = CAMO_THUMB_SIZE;
    unsigned char *px = (unsigned char *)malloc((size_t)size * size * 4);
    if (!px) return NULL;
    for (int i = 0; i < size * size; ++i) {
        px[i * 4 + 0] = 60;
        px[i * 4 + 1] = 60;
        px[i * 4 + 2] = 64;
        px[i * 4 + 3] = 255;
    }
    placeholderThumb = uiNewImage((double)size, (double)size);
    uiImageAppend(placeholderThumb, px, size, size, size * 4);
    free(px);
    return placeholderThumb;
}

static void freeCamoThumbs(void) {
    for (size_t i = 0; i < camoThumbCount; ++i) {
        if (camoThumbs[i]) uiFreeImage(camoThumbs[i]);
    }
    free(camoThumbs);
    camoThumbs = NULL;
    camoThumbCount = 0;
}

static void buildCamoThumbs(void) {
    freeCamoThumbs();
    size_t total = 0;
    const Camo *camos = camoManagerGetCamos(manager, &total);
    if (total == 0) return;
    camoThumbs = (uiImage **)calloc(total, sizeof(uiImage *));
    if (!camoThumbs) return;
    camoThumbCount = total;
    for (size_t i = 0; i < total; ++i) {
        char path[MAX_PATH];
        if (camoThumbPath(path, sizeof(path), &camos[i])) {
            camoThumbs[i] = thumbImageFromIwi(path, CAMO_THUMB_SIZE);
        }
    }
}

static int camoListNumColumns(uiTableModelHandler *mh, uiTableModel *m) {
    (void)mh; (void)m;
    return 3;
}

static uiTableValueType camoListColumnType(uiTableModelHandler *mh, uiTableModel *m,
                                           int column) {
    (void)mh; (void)m;
    return column == 0 ? uiTableValueTypeImage : uiTableValueTypeString;
}

static void camoFilesSummary(char *out, size_t size, const Camo *camo) {
    if (!out || size == 0) return;
    out[0] = '\0';
    if (!camo || camo->fileCount == 0) {
        snprintf(out, size, "-");
        return;
    }

    size_t used = 0;
    unsigned int extras = 0;
    for (int s = 0; s < CAMO_FILE_TYPE_COUNT; ++s) {
        CamoFileType type = (CamoFileType)s;
        bool present = false;
        for (size_t i = 0; i < camo->fileCount; ++i) {
            if (camo->files[i].type == type && camo->files[i].number == 0) {
                present = true;
                break;
            }
        }
        if (!present) continue;
        int n = snprintf(out + used, size - used, "%s%s",
                         used > 0 ? ", " : "", slotLabels[type]);
        if (n > 0 && (size_t)n < size - used) used += (size_t)n;
    }

    for (size_t i = 0; i < camo->fileCount; ++i) {
        if (camo->files[i].number != 0) extras++;
    }
    if (extras > 0) {
        int n = snprintf(out + used, size - used, "%s+%u",
                         used > 0 ? " " : "", extras);
        if (n > 0 && (size_t)n < size - used) used += (size_t)n;
    }

    if (out[0] == '\0') snprintf(out, size, "-");
}

static int camoListNumRows(uiTableModelHandler *mh, uiTableModel *m) {
    (void)mh; (void)m;
    return (int)camoRowCount;
}

static uiTableValue *camoListCellValue(uiTableModelHandler *mh, uiTableModel *m,
                                       int row, int column) {
    (void)mh; (void)m;
    size_t total = 0;
    const Camo *camos = camoManagerGetCamos(manager, &total);
    if (row < 0 || (size_t)row >= total) {
        return column == 0 ? NULL : uiNewTableValueString("");
    }
    if (column == 0) {
        uiImage *img = ((size_t)row < camoThumbCount && camoThumbs[row])
                           ? camoThumbs[row]
                           : getPlaceholderThumb();
        return img ? uiNewTableValueImage(img) : NULL;
    }
    if (column == 1) {
        return uiNewTableValueString(camos[row].name ? camos[row].name : "");
    }
    char summary[64];
    camoFilesSummary(summary, sizeof(summary), &camos[row]);
    return uiNewTableValueString(summary);
}

static void camoListSetCellValue(uiTableModelHandler *mh, uiTableModel *m, int row,
                                 int column, const uiTableValue *val) {
    (void)mh; (void)m; (void)row; (void)column; (void)val;
}

static void rebuildCamoList(void) {
    if (!camoListModel) return;
    size_t total = 0;
    camoManagerGetCamos(manager, &total);

    buildCamoThumbs();

    for (int i = (int)camoRowCount - 1; i >= 0; --i) {
        camoRowCount = (size_t)i;
        uiTableModelRowDeleted(camoListModel, i);
    }
    for (size_t i = 0; i < total; ++i) {
        camoRowCount = i + 1;
        uiTableModelRowInserted(camoListModel, (int)i);
    }
    camoRowCount = total;
    if (selectedCamoRow >= (int)total) selectedCamoRow = -1;
    rebuildWeaponAssignment();
}

static void appendFileRow(uiGrid *grid, int rowIndex, CamoFileType type,
                          unsigned int number, const char *source, bool enabled,
                          FileRowKey *key) {
    uiImageView *thumb = uiNewImageView(CAMO_EXTRA_SIZE, CAMO_EXTRA_SIZE);
    if (source) {
        thumbSetImageView(thumb, source, CAMO_EXTRA_SIZE);
    } else {
        thumbSetPlaceholder(thumb, CAMO_EXTRA_SIZE);
    }
    uiGridAppend(grid, uiControl(thumb), 0, rowIndex, 1, 1, 0, uiAlignStart, 0, uiAlignCenter);

    char label[64];
    camoFileLabel(label, sizeof(label), type, number);
    uiGridAppend(grid, uiControl(uiNewLabel(label)), 1, rowIndex, 1, 1, 0, uiAlignStart, 0, uiAlignCenter);

    uiButton *replace = uiNewButton(source ? "Replace" : "Add");
    uiButtonOnClicked(replace, onFileReplaceClicked, key);
    if (!enabled || !key) uiControlDisable(uiControl(replace));
    uiGridAppend(grid, uiControl(replace), 2, rowIndex, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);

    uiButton *remove = uiNewButton("Delete");
    uiButtonOnClicked(remove, onFileRemoveClicked, key);
    if (!enabled || !key || !source) uiControlDisable(uiControl(remove));
    uiGridAppend(grid, uiControl(remove), 3, rowIndex, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);
}

static void rebuildFileRows(void) {
    if (!filesContainer) return;

    if (fileRowsGrid) {
        uiBoxDelete(filesContainer, 0);
        uiControlDestroy(uiControl(fileRowsGrid));
        fileRowsGrid = NULL;
    }
    free(fileKeys);
    fileKeys = NULL;
    fileKeyCount = 0;

    fileRowsGrid = uiNewGrid();
    uiGridSetPadded(fileRowsGrid, 1);

    bool enabled = (editingCamoId != NULL);

    size_t extras = 0;
    for (size_t i = 0; i < editorFileCount; ++i) {
        if (editorFiles[i].number >= 1) extras++;
    }
    size_t rows = (size_t)CAMO_FILE_TYPE_COUNT + extras;
    fileKeys = (FileRowKey *)calloc(rows, sizeof(FileRowKey));

    size_t k = 0;

    for (int s = 0; s < CAMO_FILE_TYPE_COUNT; ++s) {
        CamoFileType type = (CamoFileType)s;
        int idx = enabled ? editorIndexOf(type, 0) : -1;
        const char *source = idx >= 0 ? editorFiles[idx].source : NULL;
        FileRowKey *key = NULL;
        if (fileKeys) {
            fileKeys[k].type = type;
            fileKeys[k].number = 0;
            key = &fileKeys[k];
        }
        appendFileRow(fileRowsGrid, (int)k, type, 0, source, enabled, key);
        k++;
    }

    for (size_t i = 0; i < editorFileCount; ++i) {
        if (editorFiles[i].number < 1) continue;
        const EditorFile *f = &editorFiles[i];
        FileRowKey *key = NULL;
        if (fileKeys) {
            fileKeys[k].type = f->type;
            fileKeys[k].number = f->number;
            key = &fileKeys[k];
        }
        appendFileRow(fileRowsGrid, (int)k, f->type, f->number, f->source, enabled, key);
        k++;
    }
    fileKeyCount = k;

    uiBoxAppend(filesContainer, uiControl(fileRowsGrid), 0);
}

static void refreshCamoDetails(void) {
    if (!detailsTitle) return;

    const Camo *camo = selectedCamo();
    char title[128];
    if (camo && camo->name) {
        snprintf(title, sizeof(title), "Camo Details: %s", camo->name);
    } else {
        snprintf(title, sizeof(title), "Camo Details");
    }
    uiLabelSetText(detailsTitle, title);

    rebuildFileRows();
}

static void onCamoListSelectionChanged(uiTable *table, void *data) {
    (void)data;
    uiTableSelection *selection = uiTableGetSelection(table);
    selectedCamoRow = (selection && selection->NumRows > 0 && selection->Rows)
                          ? selection->Rows[0]
                          : -1;
    if (selection) uiFreeTableSelection(selection);

    editorLoadFromCamo(selectedCamo());
    refreshCamoDetails();
    refreshAssignButton();
    refreshViewer();
}

static void onAddCamoClicked(uiButton *button, void *data) {
    (void)button; (void)data;
    char *name = promptText("Add Camo", "Camo name:", NULL);
    if (!name) return;
    if (name[0] == '\0') {
        uiMsgBoxError(parent, "Add camo", "Enter a name for the camo.");
        free(name);
        return;
    }
    CamoResult result = camoManagerCamoCreate(manager, name, NULL, 0);
    free(name);
    if (result != CAMO_RESULT_OK) {
        uiMsgBoxError(parent, "Add camo", "Could not create the camo.");
        return;
    }
    rebuildCamoList();
}

static void onRenameCamoClicked(uiButton *button, void *data) {
    (void)button; (void)data;
    const Camo *camo = selectedCamo();
    if (!camo) {
        uiMsgBoxError(parent, "Rename camo", "Select a camo to rename.");
        return;
    }
    char *id = _strdup(camo->id);
    char *name = promptText("Rename Camo", "Camo name:", camo->name);
    if (!name) {
        free(id);
        return;
    }
    if (name[0] == '\0') {
        uiMsgBoxError(parent, "Rename camo", "Enter a name for the camo.");
        free(name);
        free(id);
        return;
    }
    CamoFile *files = NULL;
    size_t fileCount = camo->fileCount;
    char (*sources)[MAX_PATH] = NULL;
    if (fileCount > 0) {
        files = (CamoFile *)calloc(fileCount, sizeof(CamoFile));
        sources = (char (*)[MAX_PATH])calloc(fileCount, sizeof(*sources));
        if (!files || !sources) {
            free(files);
            free(sources);
            free(name);
            free(id);
            uiMsgBoxError(parent, "Rename camo", "Out of memory.");
            return;
        }
        for (size_t i = 0; i < fileCount; ++i) {
            files[i].type = camo->files[i].type;
            files[i].number = camo->files[i].number;
            camoManagerCamoFilePath(manager, id, camo->files[i].type,
                                    camo->files[i].number, sources[i], MAX_PATH);
            files[i].fileName = sources[i];
        }
    }
    CamoResult result = camoManagerCamoUpdate(manager, id, name, files, fileCount);
    free(files);
    free(sources);
    free(name);
    if (result != CAMO_RESULT_OK) {
        uiMsgBoxError(parent, "Rename camo", "Could not rename the camo.");
        free(id);
        return;
    }
    reconcileActiveBundleRebuild(camoManagerGetActiveBundleId(manager));
    free(id);
    rebuildCamoList();
    rebuildWeaponAssignment();
    refreshCamoDetails();
}

static void onDeleteCamoClicked(uiButton *button, void *data) {
    (void)button; (void)data;
    const Camo *camo = selectedCamo();
    if (!camo) {
        uiMsgBoxError(parent, "Delete camo", "Select a camo to delete.");
        return;
    }
    char *id = _strdup(camo->id);
    if (!id) return;

    char msg[256];
    snprintf(msg, sizeof(msg), "Delete camo \"%s\"?", camo->name ? camo->name : "");
    if (uiMsgBoxOkCancel(parent, "Delete camo", msg) != 1) {
        free(id);
        return;
    }

    CamoResult result = camoManagerCamoRemove(manager, id, false);
    if (result == CAMO_RESULT_IN_USE) {
        if (uiMsgBoxOkCancel(parent, "Camo in use",
                             "This camo is assigned in one or more bundles. Delete it "
                             "and clear those assignments?") != 1) {
            free(id);
            return;
        }
        result = camoManagerCamoRemove(manager, id, true);
    }
    if (result != CAMO_RESULT_OK) {
        uiMsgBoxError(parent, "Delete camo", "Could not delete the camo.");
        free(id);
        return;
    }

    if (editingCamoId && strcmp(editingCamoId, id) == 0) {
        editorClearFiles();
        free(editingCamoId);
        editingCamoId = NULL;
    }
    reconcileActiveBundleRebuild(camoManagerGetActiveBundleId(manager));
    free(id);
    selectedCamoRow = -1;
    rebuildCamoList();
    rebuildWeaponAssignment();
    refreshCamoDetails();
}

static void onFileReplaceClicked(uiButton *button, void *data) {
    (void)button;
    if (!editingCamoId || !data) return;
    const FileRowKey *key = (const FileRowKey *)data;
    CamoFileType type = key->type;
    unsigned int number = key->number;
    char *path = uiOpenFile(parent);
    if (!path) return;
    if (!camoHasIwiExtension(path)) {
        uiMsgBoxError(parent, "Select file", "Choose an .iwi file.");
        uiFreeText(path);
        return;
    }
    if (editorSetFile(type, number, path)) {
        applyCamoFiles();
    }
    uiFreeText(path);
}

static void onFileRemoveClicked(uiButton *button, void *data) {
    (void)button;
    if (!editingCamoId || !data) return;
    const FileRowKey *key = (const FileRowKey *)data;
    editorRemoveFile(key->type, key->number);
    applyCamoFiles();
}

static void onAddExtraClicked(uiButton *button, void *data) {
    (void)button; (void)data;
    if (!editingCamoId) {
        uiMsgBoxError(parent, "Add extra file", "Select a camo first.");
        return;
    }
    int pick = promptPick("Add Extra File", "Texture role:", slotLabels,
                          CAMO_FILE_TYPE_COUNT);
    if (pick < 0) return;
    CamoFileType type = (CamoFileType)pick;

    char *path = uiOpenFile(parent);
    if (!path) return;
    if (!camoHasIwiExtension(path)) {
        uiMsgBoxError(parent, "Add extra file", "Choose an .iwi file.");
        uiFreeText(path);
        return;
    }
    unsigned int number = editorNextExtraNumber(type);
    if (number > CAMO_MAX_EXTRA) {
        uiMsgBoxError(parent, "Add extra file", "No free extra slot for that role.");
        uiFreeText(path);
        return;
    }
    if (editorSetFile(type, number, path)) {
        applyCamoFiles();
    }
    uiFreeText(path);
}

static uiTable *bundleListTable = NULL;
static uiTableModel *bundleListModel = NULL;
static uiTableModelHandler bundleListHandler;
static size_t bundleRowCount = 0;
static int selectedBundleRow = -1;
static uiButton *uninstallBtn = NULL;

static uiLabel *assignmentTitle = NULL;
static uiTable *weaponTable = NULL;
static uiTableModel *weaponModel = NULL;
static uiTableModelHandler weaponHandler;
static size_t weaponRowCount = 0;
static int selectedWeaponRow = -1;

static uiButton *assignBtn = NULL;

static int tableSelectedRow(uiTable *table) {
    if (!table) return -1;
    uiTableSelection *selection = uiTableGetSelection(table);
    int row = -1;
    if (selection && selection->NumRows > 0 && selection->Rows) row = selection->Rows[0];
    if (selection) uiFreeTableSelection(selection);
    return row;
}

static const CamoBundle *selectedBundle(void) {
    if (selectedBundleRow < 0) return NULL;
    size_t total = 0;
    const CamoBundle *bundles = camoManagerGetBundles(manager, &total);
    if ((size_t)selectedBundleRow >= total) return NULL;
    return &bundles[selectedBundleRow];
}

static const CamoWeapon *previewWeapon(void) {
    size_t total = 0;
    const CamoWeapon *weapons = camoManagerGetWeapons(manager, &total);
    if (total == 0) return NULL;
    if (selectedWeaponRow < 0 || (size_t)selectedWeaponRow >= total) return &weapons[0];
    return &weapons[selectedWeaponRow];
}

static const char *assignedCamoId(const CamoBundle *bundle, const char *weaponId) {
    if (!bundle || !weaponId) return NULL;
    for (size_t i = 0; i < bundle->entryCount; ++i) {
        if (bundle->entries[i].weaponId &&
            strcmp(bundle->entries[i].weaponId, weaponId) == 0) {
            return bundle->entries[i].camoId;
        }
    }
    return NULL;
}

static const char *camoNameById(const char *camoId) {
    if (!camoId) return "";
    size_t total = 0;
    const Camo *camos = camoManagerGetCamos(manager, &total);
    for (size_t i = 0; i < total; ++i) {
        if (camos[i].id && strcmp(camos[i].id, camoId) == 0) {
            return camos[i].name ? camos[i].name : "";
        }
    }
    return "";
}

static uiImage *camoThumbById(const char *camoId) {
    if (!camoId) return NULL;
    size_t total = 0;
    const Camo *camos = camoManagerGetCamos(manager, &total);
    for (size_t i = 0; i < total; ++i) {
        if (camos[i].id && strcmp(camos[i].id, camoId) == 0) {
            if (i < camoThumbCount && camoThumbs[i]) return camoThumbs[i];
            break;
        }
    }
    return getPlaceholderThumb();
}

static int bundleListNumColumns(uiTableModelHandler *mh, uiTableModel *m) {
    (void)mh; (void)m;
    return 3;
}

static uiTableValueType bundleListColumnType(uiTableModelHandler *mh, uiTableModel *m,
                                             int column) {
    (void)mh; (void)m; (void)column;
    return uiTableValueTypeString;
}

static int bundleListNumRows(uiTableModelHandler *mh, uiTableModel *m) {
    (void)mh; (void)m;
    return (int)bundleRowCount;
}

static uiTableValue *bundleListCellValue(uiTableModelHandler *mh, uiTableModel *m,
                                         int row, int column) {
    (void)mh; (void)m;
    size_t total = 0;
    const CamoBundle *bundles = camoManagerGetBundles(manager, &total);
    if (row < 0 || (size_t)row >= total) return uiNewTableValueString("");
    if (column == 0) {
        return uiNewTableValueString(bundles[row].name ? bundles[row].name : "");
    }
    if (column == 1) {
        char count[16];
        snprintf(count, sizeof(count), "%zu", bundles[row].entryCount);
        return uiNewTableValueString(count);
    }
    return uiNewTableValueString(isActiveBundle(bundles[row].id) ? "Installed"
                                                                 : "Not Installed");
}

static void bundleListSetCellValue(uiTableModelHandler *mh, uiTableModel *m, int row,
                                   int column, const uiTableValue *val) {
    (void)mh; (void)m; (void)row; (void)column; (void)val;
}

static int weaponNumColumns(uiTableModelHandler *mh, uiTableModel *m) {
    (void)mh; (void)m;
    return 3;
}

static uiTableValueType weaponColumnType(uiTableModelHandler *mh, uiTableModel *m,
                                         int column) {
    (void)mh; (void)m;
    return column == 1 ? uiTableValueTypeImage : uiTableValueTypeString;
}

static int weaponNumRows(uiTableModelHandler *mh, uiTableModel *m) {
    (void)mh; (void)m;
    return (int)weaponRowCount;
}

static uiTableValue *weaponCellValue(uiTableModelHandler *mh, uiTableModel *m,
                                     int row, int column) {
    (void)mh; (void)m;
    size_t total = 0;
    const CamoWeapon *weapons = camoManagerGetWeapons(manager, &total);
    if (row < 0 || (size_t)row >= total) return uiNewTableValueString("");
    const CamoWeapon *weapon = &weapons[row];
    if (column == 0) {
        return uiNewTableValueString(weapon->name ? weapon->name : weapon->id);
    }
    const CamoBundle *bundle = selectedBundle();
    const char *camoId = assignedCamoId(bundle, weapon->id);
    if (column == 1) {
        uiImage *img = camoThumbById(camoId);
        return img ? uiNewTableValueImage(img) : NULL;
    }
    return uiNewTableValueString(camoId ? camoNameById(camoId) : "-");
}

static void weaponSetCellValue(uiTableModelHandler *mh, uiTableModel *m, int row,
                               int column, const uiTableValue *val) {
    (void)mh; (void)m; (void)row; (void)column; (void)val;
}

static void refreshSelectedBundleRow(void) {
    if (bundleListModel && selectedBundleRow >= 0 &&
        (size_t)selectedBundleRow < bundleRowCount) {
        uiTableModelRowChanged(bundleListModel, selectedBundleRow);
    }
}

static void refreshAssignButton(void) {
    if (!assignBtn) return;

    const CamoBundle *bundle = selectedBundle();
    const CamoWeapon *weapon = previewWeapon();
    const Camo *camo = selectedCamo();

    const char *blocked = NULL;
    if (!weapon) blocked = "No weapons available";
    else if (!camo) blocked = "Select a camo to assign";
    else if (!bundle) blocked = "Select a bundle to assign";
    if (blocked) {
        uiButtonSetText(assignBtn, blocked);
        uiControlDisable(uiControl(assignBtn));
        return;
    }

    const char *weaponName = weapon->name ? weapon->name : weapon->id;
    const char *camoName = camo->name ? camo->name : "";
    const char *assigned = assignedCamoId(bundle, weapon->id);
    bool remove = assigned && camo->id && strcmp(assigned, camo->id) == 0;

    char label[192];
    snprintf(label, sizeof(label), remove ? "Remove \"%s\" from %s" : "Assign %s to %s",
             camoName, weaponName);
    uiButtonSetText(assignBtn, label);
    uiControlEnable(uiControl(assignBtn));
}

static void refreshActiveStatus(void) {
    for (size_t i = 0; i < bundleRowCount; ++i) {
        if (bundleListModel) uiTableModelRowChanged(bundleListModel, (int)i);
    }
    const char *installedFile = camoManagerGetInstalledBundleFile(manager);
    bool installed = installedFile && installedFile[0] != '\0';
    if (uninstallBtn) {
        if (installed) uiControlEnable(uiControl(uninstallBtn));
        else uiControlDisable(uiControl(uninstallBtn));
    }
}

static void rebuildWeaponAssignment(void) {
    if (!weaponModel) return;
    for (size_t i = 0; i < weaponRowCount; ++i) {
        uiTableModelRowChanged(weaponModel, (int)i);
    }
    if (assignmentTitle) {
        char title[128];
        const CamoBundle *bundle = selectedBundle();
        if (bundle && bundle->name) {
            snprintf(title, sizeof(title), "Camo Assignment: %s", bundle->name);
        } else {
            snprintf(title, sizeof(title), "Camo Assignment");
        }
        uiLabelSetText(assignmentTitle, title);
    }
    refreshAssignButton();
}

static void rebuildBundleList(void) {
    if (!bundleListModel) return;
    size_t total = 0;
    camoManagerGetBundles(manager, &total);
    for (int i = (int)bundleRowCount - 1; i >= 0; --i) {
        bundleRowCount = (size_t)i;
        uiTableModelRowDeleted(bundleListModel, i);
    }
    for (size_t i = 0; i < total; ++i) {
        bundleRowCount = i + 1;
        uiTableModelRowInserted(bundleListModel, (int)i);
    }
    bundleRowCount = total;
    if (selectedBundleRow >= (int)total) selectedBundleRow = -1;

    rebuildWeaponAssignment();
}

static void onBundleSelectionChanged(uiTable *table, void *data) {
    (void)data;
    selectedBundleRow = tableSelectedRow(table);
    rebuildWeaponAssignment();
}

static void onWeaponSelectionChanged(uiTable *table, void *data) {
    (void)data;
    selectedWeaponRow = tableSelectedRow(table);
    refreshAssignButton();
    refreshViewer();
}

static void onAssignClicked(uiButton *button, void *data) {
    (void)button; (void)data;

    const CamoBundle *bundle = selectedBundle();
    const CamoWeapon *weapon = previewWeapon();
    const Camo *camo = selectedCamo();
    if (!bundle || !weapon || !camo) return;

    const char *assigned = assignedCamoId(bundle, weapon->id);
    bool remove = assigned && camo->id && strcmp(assigned, camo->id) == 0;

    char *bundleId = _strdup(bundle->id);
    char *weaponId = _strdup(weapon->id);
    char *camoId = _strdup(camo->id);
    if (!bundleId || !weaponId || !camoId) {
        free(bundleId);
        free(weaponId);
        free(camoId);
        return;
    }

    CamoResult result = remove
                            ? camoManagerBundleRemoveCamo(manager, bundleId, weaponId)
                            : camoManagerBundleAddCamo(manager, bundleId, weaponId, camoId);
    free(camoId);

    if (result != CAMO_RESULT_OK) {
        uiMsgBoxError(parent, "Assign camo", "Could not update the assignment.");
        free(bundleId);
        free(weaponId);
        refreshAssignButton();
        return;
    }

    reconcileActiveBundleRebuild(bundleId);
    free(bundleId);
    free(weaponId);
    rebuildWeaponAssignment();
    refreshSelectedBundleRow();
}

static void onCreateBundleClicked(uiButton *button, void *data) {
    (void)button; (void)data;
    char *name = promptText("Create Bundle", "Bundle name:", NULL);
    if (!name) return;
    if (name[0] == '\0') {
        uiMsgBoxError(parent, "Create bundle", "Enter a name for the bundle.");
        free(name);
        return;
    }
    CamoResult result = camoManagerBundleCreate(manager, name);
    free(name);
    if (result != CAMO_RESULT_OK) {
        uiMsgBoxError(parent, "Create bundle", "Could not create the bundle.");
        return;
    }
    rebuildBundleList();
}

static void onInstallBundleClicked(uiButton *button, void *data) {
    (void)button; (void)data;
    const CamoBundle *bundle = selectedBundle();
    if (!bundle) {
        uiMsgBoxError(parent, "Install bundle", "Select a bundle to install.");
        return;
    }
    char location[MAX_PATH];
    if (!camoGameLocation(location, sizeof(location))) {
        uiMsgBoxError(parent, "Install bundle",
                      "The game location is not set. Set the Black Ops 1 game folder "
                      "before installing a camo bundle.");
        return;
    }
    CamoResult result = camoManagerBundleInstall(manager, bundle->id, location);
    if (result != CAMO_RESULT_OK) {
        uiMsgBoxError(parent, "Install bundle",
                      "Could not install the bundle. Check that the game location is "
                      "set and that its \"main\" folder exists, then try again.");
        return;
    }
    refreshActiveStatus();
    uiMsgBox(parent, "Install bundle",
             "Camo bundle installed. Black Ops 1 must be restarted for the camo "
             "changes to take effect.");
}

static void onUninstallBundleClicked(uiButton *button, void *data) {
    (void)button; (void)data;
    const char *installedFile = camoManagerGetInstalledBundleFile(manager);
    if (!installedFile || installedFile[0] == '\0') {
        uiMsgBoxError(parent, "Uninstall bundle", "No camo bundle is currently installed.");
        return;
    }
    char location[MAX_PATH];
    if (!camoGameLocation(location, sizeof(location))) {
        uiMsgBoxError(parent, "Uninstall bundle",
                      "The game location is not set. Set the Black Ops 1 game folder "
                      "before uninstalling the active camo bundle.");
        return;
    }
    CamoResult result = camoManagerBundleUninstall(manager, location);
    if (result != CAMO_RESULT_OK) {
        uiMsgBoxError(parent, "Uninstall bundle",
                      "Could not uninstall the active bundle. Check that the game "
                      "location is set and that its \"main\" folder exists, then try again.");
        return;
    }
    refreshActiveStatus();
    uiMsgBox(parent, "Uninstall bundle",
             "Camo bundle uninstalled. Black Ops 1 must be restarted for the camo "
             "changes to take effect.");
}

static void onDeleteBundleClicked(uiButton *button, void *data) {
    (void)button; (void)data;
    const CamoBundle *bundle = selectedBundle();
    if (!bundle) {
        uiMsgBoxError(parent, "Delete bundle", "Select a bundle to delete.");
        return;
    }
    char *id = _strdup(bundle->id);
    if (!id) return;

    char msg[256];
    snprintf(msg, sizeof(msg), "Delete bundle \"%s\"?", bundle->name ? bundle->name : "");
    if (uiMsgBoxOkCancel(parent, "Delete bundle", msg) != 1) {
        free(id);
        return;
    }

    if (isActiveBundle(id)) {
        char location[MAX_PATH];
        if (!camoGameLocation(location, sizeof(location))) {
            uiMsgBoxError(parent, "Delete bundle",
                          "This bundle is installed, but the game location is not set, so "
                          "its installed files can't be removed. Set the game location and "
                          "uninstall it before deleting.");
            free(id);
            return;
        }
        if (camoManagerBundleUninstall(manager, location) != CAMO_RESULT_OK) {
            uiMsgBoxError(parent, "Delete bundle",
                          "Could not uninstall the bundle before deleting it. Check that "
                          "the game location is set and that its \"main\" folder exists, "
                          "then try again.");
            free(id);
            return;
        }
    }

    CamoResult result = camoManagerBundleRemove(manager, id);
    free(id);
    if (result != CAMO_RESULT_OK) {
        uiMsgBoxError(parent, "Delete bundle", "Could not delete the bundle.");
        return;
    }

    selectedBundleRow = -1;
    rebuildBundleList();
    rebuildWeaponAssignment();
    refreshActiveStatus();
}

static void buildDetailsInto(uiGrid *grid);
static void buildAssignmentInto(uiGrid *grid);

static uiControl *buildBundlesPanel(void) {
    uiGrid *grid = uiNewGrid();
    uiGridSetPadded(grid, 1);

    uiGridAppend(grid, uiControl(uiNewLabel("Camo Bundles")),
                 0, 0, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);

    bundleListHandler.NumColumns = bundleListNumColumns;
    bundleListHandler.ColumnType = bundleListColumnType;
    bundleListHandler.NumRows = bundleListNumRows;
    bundleListHandler.CellValue = bundleListCellValue;
    bundleListHandler.SetCellValue = bundleListSetCellValue;
    bundleListModel = uiNewTableModel(&bundleListHandler);

    uiTableParams params;
    params.Model = bundleListModel;
    params.RowBackgroundColorModelColumn = -1;
    bundleListTable = uiNewTable(&params);
    uiTableSetSelectionMode(bundleListTable, uiTableSelectionModeOne);
    uiTableAppendTextColumn(bundleListTable, "Bundle", 0, uiTableModelColumnNeverEditable, NULL);
    uiTableAppendTextColumn(bundleListTable, "Camos", 1, uiTableModelColumnNeverEditable, NULL);
    uiTableAppendTextColumn(bundleListTable, "Status", 2, uiTableModelColumnNeverEditable, NULL);
    uiTableColumnSetWidth(bundleListTable, 0, 100);
    uiTableColumnSetWidth(bundleListTable, 1, 50);
    uiTableColumnSetWidth(bundleListTable, 2, 100);
    uiTableOnSelectionChanged(bundleListTable, onBundleSelectionChanged, NULL);
    uiGridAppend(grid, uiControl(bundleListTable),
                 0, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    uiBox *buttons = uiNewHorizontalBox();
    uiBoxSetPadded(buttons, 1);
    uiButton *createBtn = uiNewButton("Create");
    uiButton *installBtn = uiNewButton("Install");
    uninstallBtn = uiNewButton("Uninstall");
    uiButton *deleteBtn = uiNewButton("Delete");
    uiButtonOnClicked(createBtn, onCreateBundleClicked, NULL);
    uiButtonOnClicked(installBtn, onInstallBundleClicked, NULL);
    uiButtonOnClicked(uninstallBtn, onUninstallBundleClicked, NULL);
    uiButtonOnClicked(deleteBtn, onDeleteBundleClicked, NULL);
    uiBoxAppend(buttons, uiControl(createBtn), 1);
    uiBoxAppend(buttons, uiControl(installBtn), 1);
    uiBoxAppend(buttons, uiControl(uninstallBtn), 1);
    uiBoxAppend(buttons, uiControl(deleteBtn), 1);
    uiGridAppend(grid, uiControl(buttons),
                 0, 2, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);

    buildAssignmentInto(grid);

    rebuildBundleList();
    refreshActiveStatus();
    return uiControl(grid);
}

static uiControl *buildCamosPanel(void) {
    uiGrid *grid = uiNewGrid();
    uiGridSetPadded(grid, 1);

    uiGridAppend(grid, uiControl(uiNewLabel("Camos")),
                 0, 0, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);

    camoListHandler.NumColumns = camoListNumColumns;
    camoListHandler.ColumnType = camoListColumnType;
    camoListHandler.NumRows = camoListNumRows;
    camoListHandler.CellValue = camoListCellValue;
    camoListHandler.SetCellValue = camoListSetCellValue;
    camoListModel = uiNewTableModel(&camoListHandler);

    uiTableParams params;
    params.Model = camoListModel;
    params.RowBackgroundColorModelColumn = -1;
    camoListTable = uiNewTable(&params);
    uiTableSetSelectionMode(camoListTable, uiTableSelectionModeOne);
    uiTableAppendImageTextColumn(camoListTable, "Name", 0, 1,
                                 uiTableModelColumnNeverEditable, NULL);
    uiTableAppendTextColumn(camoListTable, "Files", 2, uiTableModelColumnNeverEditable, NULL);
    uiTableColumnSetWidth(camoListTable, 0, 115);
    uiTableColumnSetWidth(camoListTable, 1, 140);
    uiTableOnSelectionChanged(camoListTable, onCamoListSelectionChanged, NULL);
    uiGridAppend(grid, uiControl(camoListTable),
                 0, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    uiBox *buttons = uiNewHorizontalBox();
    uiBoxSetPadded(buttons, 1);
    uiButton *addBtn = uiNewButton("Create");
    uiButton *renameBtn = uiNewButton("Rename");
    uiButton *deleteBtn = uiNewButton("Delete");
    uiButtonOnClicked(addBtn, onAddCamoClicked, NULL);
    uiButtonOnClicked(renameBtn, onRenameCamoClicked, NULL);
    uiButtonOnClicked(deleteBtn, onDeleteCamoClicked, NULL);
    uiBoxAppend(buttons, uiControl(addBtn), 1);
    uiBoxAppend(buttons, uiControl(renameBtn), 1);
    uiBoxAppend(buttons, uiControl(deleteBtn), 1);
    uiGridAppend(grid, uiControl(buttons),
                 0, 2, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);

    buildDetailsInto(grid);

    rebuildCamoList();
    return uiControl(grid);
}

static void buildDetailsInto(uiGrid *grid) {
    detailsTitle = uiNewLabel("Camo Details");
    uiGridAppend(grid, uiControl(detailsTitle),
                 1, 0, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);

    filesContainer = uiNewVerticalBox();
    uiBoxSetPadded(filesContainer, 1);
    uiGridAppend(grid, uiControl(filesContainer),
                 1, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    uiButton *addExtraBtn = uiNewButton("Add Extra File...");
    uiButtonOnClicked(addExtraBtn, onAddExtraClicked, NULL);
    uiGridAppend(grid, uiControl(addExtraBtn),
                 1, 2, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);

    refreshCamoDetails();
}

static bool camoHasBaseFile(const Camo *camo, CamoFileType type) {
    if (!camo) return false;
    for (size_t i = 0; i < camo->fileCount; ++i) {
        if (camo->files[i].type == type && camo->files[i].number == 0) return true;
    }
    return false;
}

static void refreshViewerTitle(void) {
    if (!viewerGroup) return;
    const CamoWeapon *weapon = previewWeapon();
    const Camo *camo = selectedCamo();
    char title[160];
    if (weapon && camo) {
        snprintf(title, sizeof(title), "Camo Viewer: %s - %s",
                 weapon->name ? weapon->name : weapon->id, camo->name ? camo->name : "");
    } else if (weapon) {
        snprintf(title, sizeof(title), "Camo Viewer: %s",
                 weapon->name ? weapon->name : weapon->id);
    } else {
        snprintf(title, sizeof(title), "Camo Viewer");
    }
    uiGroupSetTitle(viewerGroup, title);
}

static void refreshViewer(void) {
    refreshViewerTitle();
    if (!viewer) return;

    CamoViewerRequest request;
    memset(&request, 0, sizeof(request));

    char modelPath[MAX_PATH];
    const CamoWeapon *weapon = previewWeapon();
    if (weapon &&
        camoManagerWeaponModelPath(manager, weapon->id, modelPath, sizeof(modelPath))) {
        request.modelPath = modelPath;
    }

    const Camo *camo = selectedCamo();
    char specPath[MAX_PATH], colorPath[MAX_PATH], envPath[MAX_PATH], normalPath[MAX_PATH];
    if (camo) {
        if (camoHasBaseFile(camo, CAMO_FILE_SPEC) &&
            camoManagerCamoFilePath(manager, camo->id, CAMO_FILE_SPEC, 0, specPath, sizeof(specPath))) {
            request.specPath = specPath;
        }
        if (camoHasBaseFile(camo, CAMO_FILE_COLOR) &&
            camoManagerCamoFilePath(manager, camo->id, CAMO_FILE_COLOR, 0, colorPath, sizeof(colorPath))) {
            request.colorPath = colorPath;
        }
        if (camoHasBaseFile(camo, CAMO_FILE_ENV) &&
            camoManagerCamoFilePath(manager, camo->id, CAMO_FILE_ENV, 0, envPath, sizeof(envPath))) {
            request.envPath = envPath;
        }
        if (camoHasBaseFile(camo, CAMO_FILE_NORMAL) &&
            camoManagerCamoFilePath(manager, camo->id, CAMO_FILE_NORMAL, 0, normalPath, sizeof(normalPath))) {
            request.normalPath = normalPath;
        }
    }

    camoViewerSetCamo(viewer, &request);
}

static void onViewerAutoRotateToggled(uiCheckbox *checkbox, void *data) {
    (void)data;
    if (viewer) camoViewerSetAutoRotate(viewer, uiCheckboxChecked(checkbox) != 0);
}

static const CamoViewerLayer VIEWER_LAYER_ORDER[CAMO_VIEWER_LAYER_COUNT] = {
    CAMO_VIEWER_LAYER_COLOR,
    CAMO_VIEWER_LAYER_NORMAL,
    CAMO_VIEWER_LAYER_SPEC,
    CAMO_VIEWER_LAYER_ENV,
};

static const char *VIEWER_LAYER_LABELS[CAMO_VIEWER_LAYER_COUNT] = {
    "Color",
    "Normal",
    "Spec",
    "Env",
};

static void onViewerLayerToggled(uiCheckbox *checkbox, void *data) {
    if (!viewer || !data) return;
    camoViewerSetLayerEnabled(viewer, *(const CamoViewerLayer *)data,
                              uiCheckboxChecked(checkbox) != 0);
}

static void onViewerResetClicked(uiButton *button, void *data) {
    (void)button; (void)data;
    camoViewerResetView(viewer);
}

static void onHelpClicked(uiButton *button, void *data) {
    (void)button; (void)data;
    uiCamoHelpShow(camoWindow);
}

static uiControl *buildViewer3DPanel(void) {
    viewerGroup = uiNewGroup("Camo Viewer");
    uiGroupSetMargined(viewerGroup, 1);

    uiBox *box = uiNewVerticalBox();
    uiBoxSetPadded(box, 1);

    viewerArea = uiNewGLArea(360, 320);
    uiBoxAppend(box, uiControl(viewerArea), 1);

    uiBox *controls = uiNewHorizontalBox();
    uiBoxSetPadded(controls, 1);
    for (int i = 0; i < CAMO_VIEWER_LAYER_COUNT; ++i) {
        uiCheckbox *toggle = uiNewCheckbox(VIEWER_LAYER_LABELS[i]);
        uiCheckboxSetChecked(toggle, 1);
        uiCheckboxOnToggled(toggle, onViewerLayerToggled, (void *)&VIEWER_LAYER_ORDER[i]);
        uiBoxAppend(controls, uiControl(toggle), 0);
    }
    uiCheckbox *autoRotate = uiNewCheckbox("Auto Rotate");
    uiCheckboxOnToggled(autoRotate, onViewerAutoRotateToggled, NULL);
    uiBoxAppend(controls, uiControl(autoRotate), 0);
    uiButton *resetBtn = uiNewButton("Reset View");
    uiButtonOnClicked(resetBtn, onViewerResetClicked, NULL);
    uiBoxAppend(controls, uiControl(resetBtn), 1);
    uiButton *helpBtn = uiNewButton("Help");
    uiButtonOnClicked(helpBtn, onHelpClicked, NULL);
    uiBoxAppend(controls, uiControl(helpBtn), 1);
    uiBoxAppend(box, uiControl(controls), 0);

    uiGroupSetChild(viewerGroup, uiControl(box));
    return uiControl(viewerGroup);
}

static void buildAssignmentInto(uiGrid *grid) {
    assignmentTitle = uiNewLabel("Camo Assignment");
    uiGridAppend(grid, uiControl(assignmentTitle),
                 1, 0, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);

    weaponHandler.NumColumns = weaponNumColumns;
    weaponHandler.ColumnType = weaponColumnType;
    weaponHandler.NumRows = weaponNumRows;
    weaponHandler.CellValue = weaponCellValue;
    weaponHandler.SetCellValue = weaponSetCellValue;
    weaponModel = uiNewTableModel(&weaponHandler);

    uiTableParams params;
    params.Model = weaponModel;
    params.RowBackgroundColorModelColumn = -1;
    weaponTable = uiNewTable(&params);
    uiTableSetSelectionMode(weaponTable, uiTableSelectionModeOne);
    uiTableAppendTextColumn(weaponTable, "Weapon", 0, uiTableModelColumnNeverEditable, NULL);
    uiTableAppendImageTextColumn(weaponTable, "Camo", 1, 2,
                                 uiTableModelColumnNeverEditable, NULL);
    uiTableColumnSetWidth(weaponTable, 0, 120);
    uiTableColumnSetWidth(weaponTable, 1, 145);
    uiTableOnSelectionChanged(weaponTable, onWeaponSelectionChanged, NULL);
    uiGridAppend(grid, uiControl(weaponTable),
                 1, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    assignBtn = uiNewButton("Select a camo to assign");
    uiButtonOnClicked(assignBtn, onAssignClicked, NULL);
    uiControlDisable(uiControl(assignBtn));
    uiGridAppend(grid, uiControl(assignBtn),
                 1, 2, 1, 1, 1, uiAlignFill, 0, uiAlignCenter);

    size_t total = 0;
    camoManagerGetWeapons(manager, &total);
    for (size_t i = 0; i < total; ++i) {
        weaponRowCount = i + 1;
        uiTableModelRowInserted(weaponModel, (int)i);
    }
    weaponRowCount = total;

    if (total > 0) {
        int row = 0;
        uiTableSelection selection = { 1, &row };
        uiTableSetSelection(weaponTable, &selection);
        selectedWeaponRow = 0;
    }

    rebuildWeaponAssignment();
}

static uiControl *buildContent(void) {
    uiGrid *grid = uiNewGrid();
    uiGridSetPadded(grid, 1);

    uiGroup *setup = uiNewGroup("Camo Setup");
    uiGroupSetMargined(setup, 1);
    uiBox *setupBox = uiNewVerticalBox();
    uiBoxSetPadded(setupBox, 1);
    uiBoxAppend(setupBox, buildCamosPanel(), 1);
    uiBoxAppend(setupBox, buildBundlesPanel(), 1);
    uiGroupSetChild(setup, uiControl(setupBox));

    uiGridAppend(grid, uiControl(setup),      0, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(grid, buildViewer3DPanel(), 1, 0, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

    return uiControl(grid);
}

static int onCamoWindowClose(uiWindow *window, void *data) {
    (void)data;
    uiControlHide(uiControl(window));
    return 0;
}

static void buildCamoWindow(void) {
    camoWindow = uiNewWindow(CAMO_WINDOW_TITLE, CAMO_WINDOW_WIDTH, CAMO_WINDOW_HEIGHT, 0);
    uiWindowOnClosing(camoWindow, onCamoWindowClose, NULL);
    uiWindowSetMargined(camoWindow, 1);
    uiWindowSetChild(camoWindow, buildContent());
    uiWindowSetIcon(camoWindow, IDI_ICON1);
}

void uiCamoShow(uiWindow *parentInstance) {
    parent = parentInstance;

    if (manager == NULL) {
        manager = camoManagerCreate();
        if (manager == NULL) {
            uiMsgBoxError(parent, "Camo Manager", "Failed to initialize the camo manager.");
            return;
        }
    }

    if (camoWindow == NULL) {
        buildCamoWindow();
    }

    uiControlShow(uiControl(camoWindow));
    SetForegroundWindow((HWND)uiControlHandle(uiControl(camoWindow)));

    if (viewer == NULL && viewerArea != NULL) {
        viewer = camoViewerCreate((void *)uiControlHandle(uiControl(viewerArea)));
        refreshViewer();
    }
}

void uiCamoCleanup(void) {
    if (viewer) {
        camoViewerDestroy(viewer);
        viewer = NULL;
    }
    if (manager) {
        camoManagerDestroy(manager);
        manager = NULL;
    }
}
