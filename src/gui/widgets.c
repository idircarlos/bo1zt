#include "gui/widgets.h"
#include "gui/gui_internal.h"
#include "client/widgets.h"
#include "logger.h"
#include <ui.h>
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define WIDGETS_POLL_INTERVAL_MS 250

#define N_WIDGETS 6
#define N_FONTS 21

static const char *widgetLabels[N_WIDGETS] = {
    "Timer", "Round Timer", "Velocity", "Powerup Cycle", "Zombies Left", "Entities",
};

static const char *fontNames[N_FONTS] = {
    "Digital-7 Mono", "Arial", "Times New Roman", "Calibri", "Segoe UI",
    "Tahoma", "Verdana", "Georgia", "Trebuchet MS", "Comic Sans MS",
    "Impact", "Arial Black", "Palatino Linotype", "Book Antiqua",
    "Lucida Console", "Courier New", "MS Sans Serif", "MS Serif",
    "Small Fonts", "Symbol", "Wingdings",
};

static Client *client = NULL;
static uiWindow *parent = NULL;

static WidgetConfig widgets[N_WIDGETS];

// UI Components
static uiTable *widgetTable = NULL;
static uiTableModel *widgetTableModel = NULL;
static uiColorButton *widgetColorBtn = NULL;
static uiCombobox *widgetFontCombobox = NULL;
static uiLabel *fontLabel = NULL;
static uiLabel *colorLabel = NULL;
static uiCheckbox *hideOutsideGameCheckbox = NULL;
static uiButton *btnReset = NULL;
static uiTableModelHandler tableHandler;
static uiTableParams tableParams;

static uiArea *hintArea = NULL;
static uiAreaHandler hintHandler;
static uiAttributedString *hintText = NULL;

static int selectedWidgetIndex = 0;

static ULONGLONG lastPoll = 0;

// --- Helpers ----------------------------------------------------------------

static bool colorEquals(Color a, Color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

static const char *widgetNameAt(int index) {
    return clientWidgetNameAt(index);
}

static int fontIndexOf(const char *font) {
    for (int i = 0; i < N_FONTS; i++) {
        if (strcmp(font, fontNames[i]) == 0) return i;
    }
    return 0;
}

static bool pullWidget(int index) {
    const char *name = widgetNameAt(index);
    if (!name) return false;
    return clientGetWidget(client, name, &widgets[index]) == CLIENT_OK;
}

static void pushWidget(int index) {
    const char *name = widgetNameAt(index);
    if (!name) return;
    WidgetConfig fresh;
    if (clientGetWidget(client, name, &fresh) == CLIENT_OK) {
        fresh.enabled = widgets[index].enabled;
        snprintf(fresh.font, sizeof(fresh.font), "%s", widgets[index].font);
        fresh.textColor = widgets[index].textColor;
        fresh.hideOutsideGame = widgets[index].hideOutsideGame;
        widgets[index] = fresh;
    }
    clientSetWidget(client, name, &widgets[index]);
}

// --- Table model ------------------------------------------------------------

static int tableModelNumColumns(uiTableModelHandler *mh, uiTableModel *m) {
    (void)mh; (void)m;
    return 2; // Checkbox and Name
}

static uiTableValueType tableModelColumnType(uiTableModelHandler *mh, uiTableModel *m, int column) {
    (void)mh; (void)m;
    if (column == 0) return uiTableValueTypeInt; // checkbox
    return uiTableValueTypeString;
}

static int tableModelNumRows(uiTableModelHandler *mh, uiTableModel *m) {
    (void)mh; (void)m;
    return N_WIDGETS;
}

static uiTableValue *tableModelCellValue(uiTableModelHandler *mh, uiTableModel *m, int row, int column) {
    (void)mh; (void)m;
    if (column == 0) {
        return uiNewTableValueInt(widgets[row].enabled ? 1 : 0);
    }
    return uiNewTableValueString(widgetLabels[row]);
}

static void tableModelSetCellValue(uiTableModelHandler *mh, uiTableModel *m, int row, int column, const uiTableValue *val) {
    (void)mh; (void)m;
    if (column == 0) {
        widgets[row].enabled = uiTableValueInt(val) != 0;
        pushWidget(row);
    }
}

// --- Hint area (ALT drag/resize) --------------------------------------------

static void hintHandlerDragBroken(uiAreaHandler *a, uiArea *area) { (void)a; (void)area; }
static int hintHandlerKeyEvent(uiAreaHandler *a, uiArea *area, uiAreaKeyEvent *e) { (void)a; (void)area; (void)e; return 0; }
static void hintHandlerMouseCrossed(uiAreaHandler *a, uiArea *area, int left) { (void)a; (void)area; (void)left; }
static void hintHandlerMouseEvent(uiAreaHandler *a, uiArea *area, uiAreaMouseEvent *e) { (void)a; (void)area; (void)e; }

static void hintHandlerDraw(uiAreaHandler *a, uiArea *area, uiAreaDrawParams *p) {
    (void)a; (void)area;
    if (!hintText) return;
    uiFontDescriptor font;
    uiDrawTextLayoutParams params;
    uiDrawTextLayout *layout;

    uiLoadControlFont(&font);
    params.String = hintText;
    params.DefaultFont = &font;
    params.Width = p->AreaWidth;
    params.Align = uiDrawTextAlignLeft;

    layout = uiDrawNewTextLayout(&params);
    uiDrawText(p->Context, layout, 0, 0);
    uiDrawFreeTextLayout(layout);
    uiFreeFontButtonFont(&font);
}

static uiAttributedString *buildHintAttributedString() {
    memset(&hintHandler, 0, sizeof(uiAreaHandler));
    hintHandler.Draw = hintHandlerDraw;
    hintHandler.DragBroken = hintHandlerDragBroken;
    hintHandler.KeyEvent = hintHandlerKeyEvent;
    hintHandler.MouseCrossed = hintHandlerMouseCrossed;
    hintHandler.MouseEvent = hintHandlerMouseEvent;

    uiAttributedString *attributedString = uiNewAttributedString("Hold ALT to move and resize widgets");
    size_t len = uiAttributedStringLen(attributedString);
    uiAttribute *attrItalic = uiNewItalicAttribute(uiTextItalicItalic);
    uiAttributedStringSetAttribute(attributedString, attrItalic, 0, len);
    return attributedString;
}

// --- Event handlers ---------------------------------------------------------

static void refreshCustomizationControls(int index) {
    uiComboboxSetSelected(widgetFontCombobox, fontIndexOf(widgets[index].font));
    setColorButton(widgetColorBtn, widgets[index].textColor);
    uiCheckboxSetChecked(hideOutsideGameCheckbox, widgets[index].hideOutsideGame);
}

static void onTableSelectionChanged(uiTable *table, void *data) {
    (void)data;
    uiTableSelection *selection = uiTableGetSelection(table);
    if (selection && selection->NumRows > 0 && selection->Rows) {
        selectedWidgetIndex = selection->Rows[0];
    }
    refreshCustomizationControls(selectedWidgetIndex);
    if (selection) uiFreeTableSelection(selection);
}

static void onFontChanged(uiCombobox *combobox, void *data) {
    (void)data;
    if (selectedWidgetIndex < 0) return;
    int fontIndex = uiComboboxSelected(combobox);
    if (fontIndex < 0) return;
    snprintf(widgets[selectedWidgetIndex].font, sizeof(widgets[selectedWidgetIndex].font),
             "%s", fontNames[fontIndex]);
    pushWidget(selectedWidgetIndex);
}

static void onColorChanged(uiColorButton *button, void *data) {
    (void)button; (void)data;
    if (selectedWidgetIndex < 0) return;
    widgets[selectedWidgetIndex].textColor = buildColor(widgetColorBtn);
    pushWidget(selectedWidgetIndex);
}

static void onCheckboxToggled(uiCheckbox *checkbox, void *data) {
    (void)checkbox; (void)data;
    if (selectedWidgetIndex < 0) return;
    widgets[selectedWidgetIndex].hideOutsideGame = uiCheckboxChecked(hideOutsideGameCheckbox);
    pushWidget(selectedWidgetIndex);
}

static void onResetButtonClick(uiButton *button, void *data) {
    (void)button; (void)data;
    const char *name = widgetNameAt(selectedWidgetIndex);
    if (!name) return;
    clientResetWidget(client, name);
    pullWidget(selectedWidgetIndex);
    refreshCustomizationControls(selectedWidgetIndex);
    uiTableModelRowChanged(widgetTableModel, selectedWidgetIndex);
}

// --- Build ------------------------------------------------------------------

static void init() {
    for (int i = 0; i < N_WIDGETS; i++) {
        pullWidget(i);
    }
    selectedWidgetIndex = 0;
    refreshCustomizationControls(selectedWidgetIndex);
}

uiControl *uiWidgetsBuild(Client *clientInstance, uiWindow *parentInstance) {
    client = clientInstance;
    parent = parentInstance;

    uiBox *mainBox = uiNewHorizontalBox();
    uiBoxSetPadded(mainBox, 1);

    uiBox *leftBox = uiNewVerticalBox();
    uiBoxSetPadded(leftBox, 1);

    tableHandler.NumColumns = tableModelNumColumns;
    tableHandler.ColumnType = tableModelColumnType;
    tableHandler.NumRows = tableModelNumRows;
    tableHandler.CellValue = tableModelCellValue;
    tableHandler.SetCellValue = tableModelSetCellValue;

    widgetTableModel = uiNewTableModel(&tableHandler);

    tableParams.Model = widgetTableModel;
    tableParams.RowBackgroundColorModelColumn = -1;

    widgetTable = uiNewTable(&tableParams);
    uiTableSetSelectionMode(widgetTable, uiTableSelectionModeOne);
    uiTableHeaderSetVisible(widgetTable, 0);

    uiTableAppendCheckboxColumn(widgetTable, "Enabled", 0, uiTableModelColumnAlwaysEditable);
    uiTableAppendTextColumn(widgetTable, "Widget", 1, uiTableModelColumnNeverEditable, NULL);
    uiTableColumnSetWidth(widgetTable, 0, -1);

    uiTableOnSelectionChanged(widgetTable, onTableSelectionChanged, NULL);

    uiBoxAppend(leftBox, uiControl(widgetTable), 1);

    uiBox *rightBox = uiNewVerticalBox();
    uiBoxSetPadded(rightBox, 1);

    uiGrid *customizationGrid = uiNewGrid();
    uiGridSetPadded(customizationGrid, 1);

    fontLabel = uiNewLabel("Font ");
    widgetFontCombobox = uiNewCombobox();
    for (int i = 0; i < N_FONTS; i++) {
        uiComboboxAppend(widgetFontCombobox, fontNames[i]);
    }
    uiComboboxSetSelected(widgetFontCombobox, 0);

    colorLabel = uiNewLabel("Color ");
    widgetColorBtn = uiNewColorButton();
    hideOutsideGameCheckbox = uiNewCheckbox(" Hide Outside Game");
    btnReset = uiNewButton("Reset");

    uiGridAppend(customizationGrid, uiControl(fontLabel),               0, 0, 1, 1, 0, uiAlignStart, 0, uiAlignCenter);
    uiGridAppend(customizationGrid, uiControl(widgetFontCombobox),      1, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(customizationGrid, uiControl(colorLabel),              0, 1, 1, 1, 0, uiAlignStart, 0, uiAlignCenter);
    uiGridAppend(customizationGrid, uiControl(widgetColorBtn),          1, 1, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(customizationGrid, uiControl(hideOutsideGameCheckbox), 0, 2, 2, 1, 1, uiAlignFill, 0, uiAlignFill);

    hintText = buildHintAttributedString();
    hintArea = uiNewArea(&hintHandler);
    uiAreaQueueRedrawAll(hintArea);
    uiGridAppend(customizationGrid, uiControl(hintArea),                0, 3, 2, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(customizationGrid, uiControl(btnReset),                0, 4, 2, 1, 1, uiAlignFill, 0, uiAlignFill);

    uiBoxAppend(rightBox, uiControl(customizationGrid), 1);

    uiComboboxOnSelected(widgetFontCombobox, onFontChanged, NULL);
    uiColorButtonOnChanged(widgetColorBtn, onColorChanged, NULL);
    uiCheckboxOnToggled(hideOutsideGameCheckbox, onCheckboxToggled, NULL);
    uiButtonOnClicked(btnReset, onResetButtonClick, NULL);

    uiBoxAppend(mainBox, uiControl(leftBox), 1);
    uiBoxAppend(mainBox, uiControl(rightBox), 1);

    uiBox *outerBox = uiNewVerticalBox();
    uiBoxSetPadded(outerBox, 1);
    uiBoxAppend(outerBox, uiControl(mainBox), 1);

    init();

    return uiControl(outerBox);
}

void uiWidgetsReload(void) {
    for (int i = 0; i < N_WIDGETS; i++) {
        pullWidget(i);
        uiTableModelRowChanged(widgetTableModel, i);
    }
    refreshCustomizationControls(selectedWidgetIndex);
}

static void livePoll(void) {
    ULONGLONG now = GetTickCount64();
    if (lastPoll != 0 && now - lastPoll < WIDGETS_POLL_INTERVAL_MS) return;
    lastPoll = now;

    for (int i = 0; i < N_WIDGETS; i++) {
        const char *name = widgetNameAt(i);
        if (!name) continue;

        WidgetConfig fresh;
        if (clientGetWidget(client, name, &fresh) != CLIENT_OK) continue;

        bool enabledChanged = fresh.enabled != widgets[i].enabled;
        bool controlsChanged = (i == selectedWidgetIndex) &&
            (strcmp(fresh.font, widgets[i].font) != 0 ||
             !colorEquals(fresh.textColor, widgets[i].textColor) ||
             fresh.hideOutsideGame != widgets[i].hideOutsideGame);

        widgets[i] = fresh;

        if (enabledChanged) uiTableModelRowChanged(widgetTableModel, i);
        if (controlsChanged) refreshCustomizationControls(selectedWidgetIndex);
    }
}

void uiWidgetsUpdate(void) {
    if (!parent || !uiControlVisible(uiControl(parent))) return;

    livePoll();

    uiTableSelection *selection = uiTableGetSelection(widgetTable);
    if (selection != NULL) {
        if (selection->NumRows == 0 || selection->Rows == NULL) {
            uiTableSelection currentSelection;
            int indexes[] = {selectedWidgetIndex};
            currentSelection.NumRows = 1;
            currentSelection.Rows = indexes;
            uiTableSetSelection(widgetTable, &currentSelection);
        }
        uiFreeTableSelection(selection);
    }
}
