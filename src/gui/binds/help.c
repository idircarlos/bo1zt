#include "gui/binds/help.h"
#include "logic/command/manager.h"
#include <ui.h>

#define MAX_COMMANDS 64

typedef struct {
    const char *usage;
    const char *description;
} HelpEntry;

static HelpEntry helpEntries[MAX_COMMANDS];
static int helpEntryCount = 0;

static CommandManager *commandManager = NULL;
static uiTableModelHandler tableHandler;
static uiTableModel *tableModel = NULL;

static int tableNumColumns(uiTableModelHandler *mh, uiTableModel *m) {
    (void)mh; (void)m;
    return 2;
}

static uiTableValueType tableColumnType(uiTableModelHandler *mh, uiTableModel *m, int column) {
    (void)mh; (void)m; (void)column;
    return uiTableValueTypeString;
}

static int tableNumRows(uiTableModelHandler *mh, uiTableModel *m) {
    (void)mh; (void)m;
    return helpEntryCount;
}

static uiTableValue *tableCellValue(uiTableModelHandler *mh, uiTableModel *m, int row, int column) {
    (void)mh; (void)m;
    if (row < 0 || row >= helpEntryCount) return uiNewTableValueString("");
    
    if (column == 0) {
        return uiNewTableValueString(helpEntries[row].usage);
    }
    return uiNewTableValueString(helpEntries[row].description);
}

static void tableSetCellValue(uiTableModelHandler *mh, uiTableModel *m, int row, int column, const uiTableValue *val) {
    (void)mh; (void)m; (void)row; (void)column; (void)val;
}

static void collectCommand(const CommandEntry *entry, void *userData) {
    (void)userData;
    if (helpEntryCount >= MAX_COMMANDS) return;
    helpEntries[helpEntryCount].usage = entry->usage;
    helpEntries[helpEntryCount].description = entry->description;
    helpEntryCount++;
}

static uiControl *build(Controller *controller, uiWindow *parent) {
    (void)controller;
    (void)parent;

    // Collect commands
    helpEntryCount = 0;
    commandManagerForEach(commandManager, collectCommand, NULL);

    // Setup table
    tableHandler.NumColumns = tableNumColumns;
    tableHandler.ColumnType = tableColumnType;
    tableHandler.NumRows = tableNumRows;
    tableHandler.CellValue = tableCellValue;
    tableHandler.SetCellValue = tableSetCellValue;

    tableModel = uiNewTableModel(&tableHandler);

    uiTableParams params;
    params.Model = tableModel;
    params.RowBackgroundColorModelColumn = -1;

    uiTable *table = uiNewTable(&params);
    uiTableAppendTextColumn(table, "Command", 0, uiTableModelColumnNeverEditable, NULL);
    uiTableAppendTextColumn(table, "Description", 1, uiTableModelColumnNeverEditable, NULL);
    uiTableColumnSetWidth(table, 0, 200);
    uiTableColumnSetWidth(table, 1, 250);

    return uiControl(table);
}

static void update(void) {
    // Nothing to update
}

UIControlGroup *uiBindsHelpBuildControlGroup(CommandManager *manager) {
    commandManager = manager;
    return guiControlGroupCreate(build, update);
}
