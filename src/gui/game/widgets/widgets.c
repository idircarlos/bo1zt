#include "widgets.h"
#include "../../gui_internal.h"
#include "../../../common/common.h"
#include "../../../widget/timer/timer.h"
#include "../../../widget/velocity/velocity.h"
#include "../../../map/map.h"
#include "../../../logger/logger.h"
#include "../../../timer/timer.h"
#include "../../../state/state.h"
#include <ui.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define N_WIDGETS 3
#define WIDGET_TIMER "Timer"
#define WIDGET_ROUND_TIMER "Round Timer"
#define WIDGET_VELOCITY "Velocity"

#define WIDGET_TRANSFORMING "WIDGET_TRANSFORMING"

#define N_FONTS 21

// Widget types for the table
typedef enum {
    WIDGET_NAME_TIMER,
    WIDGET_NAME_ROUND_TIMER,
    WIDGET_NAME_VELOCITY
} WidgetName;

typedef struct {
    bool enabled;
    int fontIndex;
    Color color;
    bool hideOnDefault;
    bool resetable;
    Rect rect;
    int fontSize;
} WidgetProps;

typedef struct {
    Widget *widget;
    WidgetProps status;
    char name[32];
} WidgetObj;

// Controller instance
static Controller *controller;
static uiWindow *parent;

static Map *cache = NULL;

static WidgetObj *widgets[N_WIDGETS] = { NULL, NULL, NULL };

// UI Components
static uiTable *widgetTable = NULL;
static uiTableModel *widgetTableModel = NULL;
static uiColorButton *widgetColorBtn = NULL;
static uiCombobox *widgetFontCombobox = NULL;
static uiLabel *fontLabel = NULL;
static uiLabel *colorLabel = NULL;
static uiCheckbox *hideOnDefaultCheckbox = NULL;
static uiButton *btnReset = NULL;
static uiButton *btnSave = NULL;
static uiTableModelHandler tableHandler;
static uiTableParams tableParams;

static uiArea *hintArea = NULL;
static uiAreaHandler hintHandler;
static uiAttributedString *hintText = NULL;

static int selectedWidgetIndex = 0;

static const char* widgetNames[N_WIDGETS] = {
    "Timer",
    "Round Timer",
    "Velocity"
};

static const char* widgetConfigNames[N_WIDGETS] = {
    "Timer",
    "RoundTimer",
    "Velocity"
};

static const char* fontNames[N_FONTS] = {
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
        return uiNewTableValueInt(widgets[row]->status.enabled ? 1 : 0);
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
        uiControlEnable(uiControl(btnSave));
    }
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

    uiAttributedString *attributedString = uiNewAttributedString("Hold CTRL to move and resize widgets");
    size_t len = uiAttributedStringLen(attributedString);
    uiAttribute *attrItalic = uiNewItalicAttribute(uiTextItalicItalic);
    uiAttributedStringSetAttribute(attributedString, attrItalic, 0, len);
    return attributedString;
}

static void resetWidgetStatus(WidgetObj *obj) {
    obj->status.fontIndex = 0;
    obj->status.color = colorCreate(255, 255, 255, 255);
    obj->status.hideOnDefault = false;
    obj->status.resetable = false;
    widgetSetFont(obj->widget,fontNames[obj->status.fontIndex]);
    widgetSetTextColor(obj->widget, obj->status.color);
}

static WidgetObj *createWidgetObj(WidgetName widgetName, Widget *widget) {
    WidgetObj *obj = (WidgetObj *)malloc(sizeof(WidgetObj));
    if (!obj) return NULL;
    const char *name = widgetNames[widgetName];
    strcpy(obj->name, name);
    obj->status.enabled = false;
    obj->status.fontIndex = 0;
    obj->status.color = colorCreate(255, 255, 255, 255);
    obj->status.hideOnDefault = false;
    obj->status.resetable = false;
    obj->widget = widget;
    widgets[widgetName] = obj; 
    return obj;
}

static void updateWidgetObj(WidgetObj *obj, WidgetProps props) {
    obj->status.enabled = props.enabled;
    obj->status.fontIndex = props.fontIndex;
    obj->status.color = props.color;
    obj->status.hideOnDefault = props.hideOnDefault;
    obj->status.resetable = props.resetable;
    obj->status.rect = props.rect;
    obj->status.fontSize = props.fontSize;
    widgetSetFont(obj->widget, fontNames[obj->status.fontIndex]);
    widgetSetTextColor(obj->widget, obj->status.color);
    widgetSetPosition(obj->widget, obj->status.rect);
    widgetSetFontSize(obj->widget, obj->status.fontSize);
    props.enabled ? widgetShow(obj->widget) : widgetHide(obj->widget);
}

// Event handlers
static void onTableSelectionChanged(uiTable *table, void *data) {
    (void)data;
    uiTableSelection *selection = uiTableGetSelection(table);
    if (selection && selection->NumRows > 0 && selection->Rows) {
        selectedWidgetIndex = selection->Rows[0]; // Get first selected row
    }
    WidgetObj *obj = widgets[selectedWidgetIndex];
    uiComboboxSetSelected(widgetFontCombobox, obj->status.fontIndex);
    setColorButton(widgetColorBtn, obj->status.color);
    uiCheckboxSetChecked(hideOnDefaultCheckbox, obj->status.hideOnDefault);
    if (obj->status.resetable) uiControlEnable(uiControl(btnReset));
    else uiControlDisable(uiControl(btnReset));
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
            widgets[selectedWidgetIndex]->status.fontIndex = fontIndex;
            widgetSetFont(widgets[selectedWidgetIndex]->widget, fontNames[fontIndex]);
        }
        widgets[selectedWidgetIndex]->status.resetable = true;
        uiControlEnable(uiControl(btnReset));
        uiControlEnable(uiControl(btnSave));
    }
}

static void onColorChanged(uiColorButton *button, void *data) {
    (void)button;
    (void)data;
    // TODO: Apply Color on update()
    if (selectedWidgetIndex >= 0) {
        widgets[selectedWidgetIndex]->status.color = buildColor(widgetColorBtn);
        widgetSetTextColor(widgets[selectedWidgetIndex]->widget, widgets[selectedWidgetIndex]->status.color);
        widgets[selectedWidgetIndex]->status.resetable = true;
        uiControlEnable(uiControl(btnReset));
        uiControlEnable(uiControl(btnSave));
    }
}

static void onCheckboxToggled(uiCheckbox *checkbox, void *data) {
    (void)checkbox;
    (void)data;
    // TODO: Show/Hide on update()
    if (selectedWidgetIndex >= 0) {
        bool hideOnDefault = uiCheckboxChecked(hideOnDefaultCheckbox);
        widgets[selectedWidgetIndex]->status.hideOnDefault = hideOnDefault;
        widgets[selectedWidgetIndex]->status.resetable = true;
        uiControlEnable(uiControl(btnReset));
        uiControlEnable(uiControl(btnSave));
    }
}

static void onResetButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    WidgetObj *obj = widgets[selectedWidgetIndex];
    resetWidgetStatus(obj);
    controllerWidgetResetConfig(controller, selectedWidgetIndex);
    uiComboboxSetSelected(widgetFontCombobox, 0);
    setColorButton(widgetColorBtn, colorCreate(255, 255, 255, 255));
    uiCheckboxSetChecked(hideOnDefaultCheckbox, false);
    uiControlDisable(uiControl(btnReset));
    uiControlEnable(uiControl(btnSave));
    obj->status.resetable = false;
}

static void onSaveButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    controllerUpdateConfig(controller, CONFIG_WIDGETS);
    uiControlDisable(uiControl(btnSave));
}

static WidgetProps getWidgetPropsFromConfig(int index) {
    WidgetConfig widgetConfig = controllerGetWidgetConfig(controller, index);
    int fontIndex = 0;
    for (int j = 0; j < N_FONTS; j++) {
        if (strcmp(widgetConfig.font, fontNames[j]) == 0) {
            fontIndex = j;
            break;
        }
    }
    WidgetProps props = {
        widgetConfig.enabled,
        fontIndex,
        widgetConfig.textColor,
        widgetConfig.hideOnDefault,
        false,
        widgetConfig.rect,
        widgetConfig.fontSize,
    };
    return props;
}

static void init() {
    for (int i = 0; i < N_WIDGETS; i++) {
        WidgetProps props = getWidgetPropsFromConfig(i);
        updateWidgetObj(widgets[i], props);
    }
    uiComboboxSetSelected(widgetFontCombobox, widgets[WIDGET_NAME_TIMER]->status.fontIndex);
    setColorButton(widgetColorBtn, widgets[WIDGET_NAME_TIMER]->status.color);
    selectedWidgetIndex = 0;
    uiControlDisable(uiControl(btnSave));
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
    btnReset = uiNewButton("Reset");

    uiControlDisable(uiControl(btnReset));
    
    uiGridAppend(customizationGrid, uiControl(fontLabel),               0, 0, 1, 1, 0, uiAlignStart, 0, uiAlignCenter);
    uiGridAppend(customizationGrid, uiControl(widgetFontCombobox),      1, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(customizationGrid, uiControl(colorLabel),              0, 1, 1, 1, 0, uiAlignStart, 0, uiAlignCenter);
    uiGridAppend(customizationGrid, uiControl(widgetColorBtn),          1, 1, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(customizationGrid, uiControl(hideOnDefaultCheckbox),   0, 2, 2, 1, 1, uiAlignFill, 0, uiAlignFill);
    
    
    hintText = buildHintAttributedString();

    hintArea = uiNewArea(&hintHandler);
    
    uiAreaQueueRedrawAll(hintArea);
    uiGridAppend(customizationGrid, uiControl(hintArea),                0, 3, 2, 1, 1, uiAlignFill, 1, uiAlignFill);
    uiGridAppend(customizationGrid, uiControl(btnReset),                0, 4, 2, 1, 1, uiAlignFill, 0, uiAlignFill);
    

    uiBoxAppend(rightBox, uiControl(customizationGrid), 1);

    uiComboboxOnSelected(widgetFontCombobox, onFontChanged, NULL);
    uiColorButtonOnChanged(widgetColorBtn, onColorChanged, NULL);
    uiCheckboxOnToggled(hideOnDefaultCheckbox, onCheckboxToggled, NULL);
    uiButtonOnClicked(btnReset, onResetButtonClick, NULL);

    uiBoxAppend(mainBox, uiControl(leftBox), 1);
    uiBoxAppend(mainBox, uiControl(rightBox), 1);

    btnSave = uiNewButton("Save");
    
    uiButtonOnClicked(btnSave, onSaveButtonClick, NULL);

    uiBox *outerBox = uiNewVerticalBox();
    uiBoxSetPadded(outerBox, 1);
    uiBoxAppend(outerBox, uiControl(mainBox), 1);
    uiBoxAppend(outerBox, uiControl(btnSave), 0);
 
    State *state = controllerGetState(controller);
    Timer *timerInstance = stateGetTimer(state);
    Timer *roundTimerInstance = stateGetRoundTimer(state);

    createWidgetObj(WIDGET_NAME_TIMER, timerWidgetCreate(timerInstance));
    createWidgetObj(WIDGET_NAME_ROUND_TIMER, timerWidgetCreate(roundTimerInstance));
    createWidgetObj(WIDGET_NAME_VELOCITY, velocityWidgetCreate());
    cache = mapCreate();
    mapPutBool(cache, WIDGET_TRANSFORMING, false);
    init();

    return uiControl(outerBox);
}

static void update() {
    // For some reason uiTableSelectionModeOne does not work properly when clicking on an empty area of the table, having 0 row selected.
    // As a workaround, if user is deselecting any row, keep focusing to that one. Do it only when the window is visible.
    if (uiControlVisible(uiControl(parent))) {
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
    
    bool wasTransforming = mapGetBool(cache, WIDGET_TRANSFORMING);
    bool isTransformingNow = false;
    for (int i = 0; i < N_WIDGETS; i++) {
        if (widgetIsTransforming(widgets[i]->widget)) {
            isTransformingNow = true;
            break;
        }
    }
    if (wasTransforming && !isTransformingNow) {
        controllerUpdateConfig(controller, CONFIG_WIDGETS);
    }
    mapPutBool(cache, WIDGET_TRANSFORMING, isTransformingNow);
}

// External API for Controller
UIControlGroup *uiWidgetsBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}

const char *uiWidgetsGetName(int index) {
    if (index < 0 || index >= N_WIDGETS) return NULL;
    return widgetConfigNames[index];
}

bool uiWidgetsIsEnabled(int index) {
    if (index < 0 || index >= N_WIDGETS) return false;
    return widgets[index]->status.enabled;
}

const char *uiWidgetsGetFont(int index) {
    if (index < 0 || index >= N_WIDGETS) return NULL;
    int fontIndex = widgets[index]->status.fontIndex;
    if (fontIndex < 0 || fontIndex >= N_FONTS) return NULL;
    return fontNames[fontIndex];
}

Color uiWidgetsGetTextColor(int index) {
    if (index < 0 || index >= N_WIDGETS) return colorCreate(255, 255, 255, 255);
    return widgets[index]->status.color;
}

bool uiWidgetsIsHideOnDefaultChecked(int index) {
    if (index < 0 || index >= N_WIDGETS) return false;
    return widgets[index]->status.hideOnDefault;
}

bool uiWidgetsIsSavable() {
    return uiControlEnabled(uiControl(btnSave));
}

void uiWidgetsReset() {
    init();
}

Rect uiWidgetsGetRect(int index) {
    return widgetGetPosition(widgets[index]->widget);
}

int uiWidgetsGetFontSize(int index) {
    return widgetGetFontSize(widgets[index]->widget);
}

Rect uiWidgetsGetDefaultRect(int index) {
    switch(index) {
        case WIDGET_NAME_TIMER:
        case WIDGET_NAME_ROUND_TIMER:
            return WIDGET_TIMER_RECT;
        case WIDGET_NAME_VELOCITY:
            return WIDGET_VELOCITY_RECT;
        default:
            LOG_ERROR("Unknown widget index %d\n", index);
            return rectCreate(0, 0, 0, 0);
    }
}

int uiWidgetsGetDefaultFontSize(int index) {
    switch(index) {
        case WIDGET_NAME_TIMER:
        case WIDGET_NAME_ROUND_TIMER:
            return WIDGET_TIMER_FONT_SIZE;
        case WIDGET_NAME_VELOCITY:
            return WIDGET_VELOCITY_FONT_SIZE;
        default:
            LOG_ERROR("Unknown widget index %d\n", index);
            return 0;
    }
}
