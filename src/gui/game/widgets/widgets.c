#include "widgets.h"
#include "../../../widget/timer/timer.h"
#include "../../../widget/velocity/velocity.h"
#include "../../../widget/fonts.h"
#include "../../../../res/resource_ids.h"
#include "../../../logger/logger.h"
#include <ui.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Widget types for the table
typedef enum {
    WIDGET_TYPE_TIMER,
    WIDGET_TYPE_ROUND_TIMER,
    WIDGET_TYPE_VELOCITY
} WidgetType;

typedef struct {
    bool enabled;
    int fontIndex;
    Color color;
    bool hideOnDefault;
} WidgetProps;

typedef struct {
    Widget *widget;
    WidgetProps status;
    char name[32];
} WidgetObj;

// Controller instance
static Controller *controller;
static uiWindow *parent;

static WidgetObj *widgets[N_WIDGETS] = { NULL, NULL, NULL };

// UI Components
static uiTable *widgetTable = NULL;
static uiTableModel *widgetTableModel = NULL;
static uiColorButton *widgetColorBtn = NULL;
static uiCombobox *widgetFontCombobox = NULL;
static uiLabel *fontLabel = NULL;
static uiLabel *colorLabel = NULL;
static uiCheckbox *hideOnDefaultCheckbox = NULL;
static uiButton *uiWidgetsBtnReset = NULL;
static uiButton *uiWidgetsBtnSave = NULL;
static uiTableModelHandler tableHandler;
static uiTableParams tableParams;

static uiArea *hintArea = NULL;
static uiAreaHandler hintHandler;
static uiAttributedString *hintText = NULL;

static int selectedWidgetIndex = 0;

// Font list - Digital-7 Mono primero (default) + 20 fuentes principales de Windows
static const char* fontNames[] = {
    "Digital-7 Mono",
    "Arial",
    "Times New Roman",
    "Calibri",
    "Segoe UI",
    "Tahoma",
    "Verdana",
    "Georgia",
    "Trebuchet MS",
    "Comic Sans MS",
    "Impact",
    "Arial Black",
    "Palatino Linotype",
    "Book Antiqua",
    "Lucida Console",
    "Courier New",
    "MS Sans Serif",
    "MS Serif",
    "Small Fonts",
    "Symbol",
    "Wingdings"
};

// Table model functions
static int tableModelNumColumns(uiTableModelHandler *mh, uiTableModel *m) {
    (void)mh;
    (void)m;
    return 2; // Checkbox and Name
}

static uiTableValueType tableModelColumnType(uiTableModelHandler *mh, uiTableModel *m, int column) {
    (void)mh;
    (void)m;
    if (column == 0) {
        return uiTableValueTypeInt; // For checkbox

    }
    return uiTableValueTypeString; // For widget name
}

static int tableModelNumRows(uiTableModelHandler *mh, uiTableModel *m) {
    (void)mh;
    (void)m;
    return N_WIDGETS;
}

static uiTableValue *tableModelCellValue(uiTableModelHandler *mh, uiTableModel *m, int row, int column) {
    (void)mh;
    (void)m;
    
    if (column == 0) {
        // Checkbox column
        return uiNewTableValueInt(widgets[row]->status.enabled);
    } else {
        // Widget column
        return uiNewTableValueString(widgets[row]->name);
    }
}

static void tableModelSetCellValue(uiTableModelHandler *mh, uiTableModel *m, int row, int column, const uiTableValue *val) {
    (void)mh;
    (void)m;
    // Checkbox column
    if (column == 0) {
        bool checked = uiTableValueInt(val) != 0;
        widgets[row]->status.enabled = checked;
        checked ? widgetShow(widgets[row]->widget) : widgetHide(widgets[row]->widget);
    }
}

// Aux
static Color buildColor(uiColorButton *button) {
    double r, g, b, a;
    uiColorButtonColor(button, &r, &g, &b, &a);
    Color color = colorCreate(r*255, g*255, b*255, a*255);
    return color;
}

static void setColorButton(uiColorButton *button, Color color) {
    uiColorButtonSetColor(button, color.r / 255.0, color.g / 255.0, color.b / 255.0, 1.0);
}

static void hintHandlerDragBroken(uiAreaHandler *a, uiArea *area) { (void)a; (void)area; }
static int hintHandlerKeyEvent(uiAreaHandler *a, uiArea *area, uiAreaKeyEvent *e) { (void)a; (void)area; (void)e; return 0; }
static void hintHandlerMouseCrossed(uiAreaHandler *a, uiArea *area, int left) { (void)a; (void)area; (void)left; }
static void hintHandlerMouseEvent(uiAreaHandler *a, uiArea *area, uiAreaMouseEvent *e) { (void)a; (void)area; (void)e; }

static void hintHandlerDraw(uiAreaHandler *a, uiArea *area, uiAreaDrawParams *p) {
    (void)a;
    (void)area;
    if (!hintText)
        return;
    uiFontDescriptor font;
    uiDrawTextLayoutParams params;
    uiDrawTextLayout *layout;

    uiLoadControlFont(&font);
    params.String = hintText;
    params.DefaultFont = &font;
    params.Width = p->AreaWidth;
    params.Align = uiDrawTextAlignLeft;

    layout = uiDrawNewTextLayout(&params);
    uiDrawText(p->Context, layout, 0, -0.5);
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

    uiAttributedString *attributedString = uiNewAttributedString("Hold CTRL to move and resize widgets");
    size_t len = uiAttributedStringLen(attributedString);
    uiAttribute *attrItalic = uiNewItalicAttribute(uiTextItalicItalic);
    uiAttributedStringSetAttribute(attributedString, attrItalic, 0, len);
    return attributedString;
}

// Event handlers
static void onTableSelectionChanged(uiTable *table, void *data) {
    (void)data;
    uiTableSelection *selection = uiTableGetSelection(table);
    if (selection && selection->NumRows > 0 && selection->Rows) {
        selectedWidgetIndex = selection->Rows[0]; // Get first selected row
    }
    
    uiComboboxSetSelected(widgetFontCombobox, widgets[selectedWidgetIndex]->status.fontIndex);
    setColorButton(widgetColorBtn, widgets[selectedWidgetIndex]->status.color);
    uiCheckboxSetChecked(hideOnDefaultCheckbox, widgets[selectedWidgetIndex]->status.hideOnDefault);
    
    if (selection) {
        uiFreeTableSelection(selection);
    }
}

static void onFontChanged(uiCombobox *combobox, void *data) {
    (void)combobox;
    (void)data;
    // TODO: Apply Font on update()
    if (selectedWidgetIndex >= 0) {
        int fontIndex = uiComboboxSelected(combobox);
        if (fontIndex >= 0) {
            LOG_INFO("Font changed for widget %s to %s\n", 
                     widgets[selectedWidgetIndex]->name, fontNames[fontIndex]);
            widgets[selectedWidgetIndex]->status.fontIndex = fontIndex;
            widgetSetFont(widgets[selectedWidgetIndex]->widget, fontNames[fontIndex]);
        }
    }
}

static void onColorChanged(uiColorButton *button, void *data) {
    (void)button;
    (void)data;
    // TODO: Apply Color on update()
    if (selectedWidgetIndex >= 0) {
        LOG_INFO("Color changed for widget %s\n", widgets[selectedWidgetIndex]->name);
        widgets[selectedWidgetIndex]->status.color = buildColor(widgetColorBtn);
        widgetSetTextColor(widgets[selectedWidgetIndex]->widget, widgets[selectedWidgetIndex]->status.color);
    }
}

static void onCheckboxToggled(uiCheckbox *checkbox, void *data) {
    (void)checkbox;
    (void)data;
    // TODO: Show/Hide on update()
    if (selectedWidgetIndex >= 0) {
        bool hideOnDefault = uiCheckboxChecked(hideOnDefaultCheckbox);
        LOG_INFO("Hide on Default changed for widget %s to %s\n", 
                 widgets[selectedWidgetIndex]->name, hideOnDefault ? "true" : "false");
        widgets[selectedWidgetIndex]->status.hideOnDefault = hideOnDefault;
    }
}

static WidgetObj *createWidgetObj(const char *name, Widget *widget, WidgetProps props) {
    WidgetObj *obj = (WidgetObj *)malloc(sizeof(WidgetObj));
    if (!obj) return NULL;
    strcpy(obj->name, name);
    obj->widget = widget;
    obj->status.enabled = props.enabled;
    obj->status.fontIndex = props.fontIndex;
    obj->status.color = props.color;
    obj->status.hideOnDefault = props.hideOnDefault;
    widgetSetFont(widget, fontNames[props.fontIndex]);
    widgetSetTextColor(widget, props.color);
    props.enabled ? widgetShow(widget) : widgetHide(widget);
    return obj;
}

static void init() {
    fontsInit();
    fontsLoad(IDR_FONT_DIGITAL_7_MONO);
    WidgetProps props = { false, 0, COLOR_WHITE, false };
    WidgetObj *timerWidget = createWidgetObj(WIDGET_TIMER, timerWidgetCreate(200, 200), props);
    WidgetObj *roundTimerWidget = createWidgetObj(WIDGET_ROUND_TIMER, timerWidgetCreate(200, 400), props);
    WidgetObj *velocityWidget = createWidgetObj(WIDGET_VELOCITY, velocityWidgetCreate(600, 200), props);

    widgets[WIDGET_TYPE_TIMER] = timerWidget;
    widgets[WIDGET_TYPE_ROUND_TIMER] = roundTimerWidget;
    widgets[WIDGET_TYPE_VELOCITY] = velocityWidget;

    uiComboboxSetSelected(widgetFontCombobox, props.fontIndex);
    setColorButton(widgetColorBtn, props.color);
}

static uiControl *build(Controller *controllerInstance, uiWindow *parentInstance) {
    controller = controllerInstance;
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

    for (int i = 0; i < 21; i++) {
        uiComboboxAppend(widgetFontCombobox, fontNames[i]);
    }
    uiComboboxSetSelected(widgetFontCombobox, 0);
    
    colorLabel = uiNewLabel("Color ");
    widgetColorBtn = uiNewColorButton();
    hideOnDefaultCheckbox = uiNewCheckbox(" Hide on Default");
    
    uiGridAppend(customizationGrid, uiControl(fontLabel),               0, 0, 1, 1, 0, uiAlignStart, 0, uiAlignCenter);
    uiGridAppend(customizationGrid, uiControl(widgetFontCombobox),      1, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(customizationGrid, uiControl(colorLabel),              0, 1, 1, 1, 0, uiAlignStart, 0, uiAlignCenter);
    uiGridAppend(customizationGrid, uiControl(widgetColorBtn),          1, 1, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(customizationGrid, uiControl(hideOnDefaultCheckbox),   0, 2, 2, 1, 1, uiAlignFill, 0, uiAlignFill);
    
    hintText = buildHintAttributedString();

    hintArea = uiNewArea(&hintHandler);
    uiAreaQueueRedrawAll(hintArea);
    uiGridAppend(customizationGrid, uiControl(hintArea), 0, 3, 2, 1, 1, uiAlignFill, 1, uiAlignFill);
    

    uiBoxAppend(rightBox, uiControl(customizationGrid), 1);

    uiComboboxOnSelected(widgetFontCombobox, onFontChanged, NULL);
    uiColorButtonOnChanged(widgetColorBtn, onColorChanged, NULL);
    uiCheckboxOnToggled(hideOnDefaultCheckbox, onCheckboxToggled, NULL);

    uiBoxAppend(mainBox, uiControl(leftBox), 1);
    uiBoxAppend(mainBox, uiControl(rightBox), 1);

    uiBox *buttonBox = uiNewHorizontalBox();
    uiBoxSetPadded(buttonBox, 1);

    uiWidgetsBtnReset = uiNewButton("Reset");
    uiWidgetsBtnSave = uiNewButton("Save");

    uiControlDisable(uiControl(uiWidgetsBtnReset));
    uiControlDisable(uiControl(uiWidgetsBtnSave));

    uiBoxAppend(buttonBox, uiControl(uiWidgetsBtnReset), 1);
    uiBoxAppend(buttonBox, uiControl(uiWidgetsBtnSave), 1);

    uiBox *outerBox = uiNewVerticalBox();
    uiBoxSetPadded(outerBox, 1);
    uiBoxAppend(outerBox, uiControl(mainBox), 1);
    uiBoxAppend(outerBox, uiControl(buttonBox), 0);

    init();

    return uiControl(outerBox);
}

static void update() {
    // For some reason uiTableSelectionModeOne does not work properly when clicking on an empty area of the table, having 0 row selected.
    // As a workaround, if user is deselecting any row, keep focusing to that one.
    uiTableSelection *selection = uiTableGetSelection(widgetTable);
    if (!selection->Rows) {
        uiTableSelection currentSelection;
        int indexes[] = {selectedWidgetIndex};
        currentSelection.NumRows = 1;      // Preselect only one row
        currentSelection.Rows = indexes;   // The current index
        uiTableSetSelection(widgetTable, &currentSelection);
    }
    if (selection) uiFreeTableSelection(selection);
}

// External API for Controller
UIControlGroup *uiWidgetsBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}
