#include "gui/binds/help.h"
#include "client/commands.h"
#include <ui.h>

#define MAX_COMMANDS 64

static CommandInfo commands[MAX_COMMANDS];
static int commandCount = 0;

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
    return commandCount;
}

static uiTableValue *tableCellValue(uiTableModelHandler *mh, uiTableModel *m, int row, int column) {
    (void)mh; (void)m;
    if (row < 0 || row >= commandCount) return uiNewTableValueString("");

    if (column == 0) {
        return uiNewTableValueString(commands[row].usage);
    }
    return uiNewTableValueString(commands[row].description);
}

static void tableSetCellValue(uiTableModelHandler *mh, uiTableModel *m, int row, int column, const uiTableValue *val) {
    (void)mh; (void)m; (void)row; (void)column; (void)val;
}

uiControl *uiBindsHelpBuild(Client *client, uiWindow *parent) {
    (void)parent;

    commandCount = 0;
    clientGetCommands(client, commands, MAX_COMMANDS, &commandCount);

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
