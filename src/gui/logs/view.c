#include "gui/logs/view.h"

#include <Scintilla.h>

#include <stdlib.h>
#include <string.h>

#include "utils/color.h"

#define LOGS_VIEW_FONT "Consolas"
#define LOGS_VIEW_FONT_SIZE 9

#define LOGS_STYLE_MESSAGE 0
#define LOGS_STYLE_TIMESTAMP 7
#define LOGS_STYLE_ORIGIN 8
#define logsStyleOf(level) ((int)(level) + 1)

static const RGBColor LOGS_COLOR_MESSAGE = {0x30, 0x30, 0x30};    // near black
static const RGBColor LOGS_COLOR_TIMESTAMP = {0xC0, 0x5A, 0x0A};  // copper
static const RGBColor LOGS_COLOR_ORIGIN = {0xC2, 0x14, 0x8F};     // magenta

static const RGBColor LOGS_LEVEL_COLOR[] = {
    {0x9A, 0x9A, 0x9A},  // TRACE: grey
    {0x1C, 0x8A, 0x1C},  // DEBUG: green
    {0x00, 0x60, 0xC0},  // INFO:  blue
    {0xC8, 0x92, 0x0F},  // WARN:  amber
    {0xD0, 0x22, 0x22},  // ERROR: red
    {0x8B, 0x00, 0x00},  // FATAL: dark red
};

struct LogsView {
    uiScintilla *editor;
    LogsTail *tail;
    LogsLevel minimum;
    bool autoscroll;
    bool hideTimestamp;
    bool hideOrigin;
    bool currentRunOnly;
    size_t runStart;
    size_t rendered;
};

// Scintilla colours are 0xBBGGRR, not 0xRRGGBB
static void setStyleColor(uiScintilla *editor, int style, RGBColor color) {
    intptr_t packed = color.r | (color.g << 8) | (color.b << 16);
    uiScintillaSend(editor, SCI_STYLESETFORE, style, packed);
}

static void configure(uiScintilla *editor) {
    uiScintillaSend(editor, SCI_STYLESETFONT, STYLE_DEFAULT, (intptr_t)LOGS_VIEW_FONT);
    uiScintillaSend(editor, SCI_STYLESETSIZE, STYLE_DEFAULT, LOGS_VIEW_FONT_SIZE);
    uiScintillaSend(editor, SCI_STYLECLEARALL, 0, 0);

    setStyleColor(editor, LOGS_STYLE_MESSAGE, LOGS_COLOR_MESSAGE);
    setStyleColor(editor, LOGS_STYLE_TIMESTAMP, LOGS_COLOR_TIMESTAMP);
    setStyleColor(editor, LOGS_STYLE_ORIGIN, LOGS_COLOR_ORIGIN);
    uiScintillaSend(editor, SCI_STYLESETBOLD, LOGS_STYLE_TIMESTAMP, 1);
    uiScintillaSend(editor, SCI_STYLESETBOLD, LOGS_STYLE_ORIGIN, 1);
    for (int level = 0; level < LOGS_LEVEL_UNTAGGED; level++) {
        setStyleColor(editor, logsStyleOf(level), LOGS_LEVEL_COLOR[level]);
        uiScintillaSend(editor, SCI_STYLESETBOLD, logsStyleOf(level), 1);
    }
    uiScintillaSend(editor, SCI_SETMARGINWIDTHN, 0, 0);
    uiScintillaSend(editor, SCI_SETMARGINWIDTHN, 1, 0);
    uiScintillaSend(editor, SCI_SETEOLMODE, SC_EOL_LF, 0);
    uiScintillaSend(editor, SCI_SETSCROLLWIDTHTRACKING, 1, 0);
    uiScintillaSend(editor, SCI_SETUNDOCOLLECTION, 0, 0);
    uiScintillaSend(editor, SCI_SETREADONLY, 1, 0);
}

static void appendRun(LogsView *view, const char *text, size_t length, int style) {
    if (length == 0) return;

    intptr_t start = uiScintillaSend(view->editor, SCI_GETLENGTH, 0, 0);

    uiScintillaSend(view->editor, SCI_APPENDTEXT, length, (intptr_t)text);
    uiScintillaSend(view->editor, SCI_STARTSTYLING, (uintptr_t)start, 0);
    uiScintillaSend(view->editor, SCI_SETSTYLING, length, style);
}

static void appendLine(LogsView *view, const LogsLine *line) {
    size_t length = strlen(line->text);

    if (line->level == LOGS_LEVEL_UNTAGGED) {
        appendRun(view, line->text, length, LOGS_STYLE_MESSAGE);
        appendRun(view, "\n", 1, LOGS_STYLE_MESSAGE);
        return;
    }

    size_t levelEnd = line->tokenAt + LOGS_LEVEL_TOKEN_SIZE;
    size_t originLength = line->messageAt - levelEnd;

    if (!view->hideTimestamp) appendRun(view, line->text, line->tokenAt, LOGS_STYLE_TIMESTAMP);
    appendRun(view, line->text + line->tokenAt, LOGS_LEVEL_TOKEN_SIZE, logsStyleOf(line->level));

    if (!view->hideOrigin) appendRun(view, line->text + levelEnd, originLength, LOGS_STYLE_ORIGIN);
    else if (originLength > 0) appendRun(view, " ", 1, LOGS_STYLE_MESSAGE);

    appendRun(view, line->text + line->messageAt, length - line->messageAt, LOGS_STYLE_MESSAGE);
    appendRun(view, "\n", 1, LOGS_STYLE_MESSAGE);
}

static void appendFrom(LogsView *view, size_t from) {
    size_t count = logsTailCount(view->tail);

    uiScintillaSend(view->editor, SCI_SETREADONLY, 0, 0);
    for (size_t i = from; i < count; i++) {
        const LogsLine *line = logsTailAt(view->tail, i);
        if (!line->header && line->level >= view->minimum) appendLine(view, line);
    }
    uiScintillaSend(view->editor, SCI_SETREADONLY, 1, 0);

    view->rendered = logsTailTotal(view->tail);
    if (view->autoscroll) {
        uiScintillaSend(view->editor, SCI_GOTOPOS,
                        (uintptr_t)uiScintillaSend(view->editor, SCI_GETLENGTH, 0, 0), 0);
    }
}

static size_t firstVisible(const LogsView *view) {
    if (!view->currentRunOnly) return 0;

    size_t oldest = logsTailTotal(view->tail) - logsTailCount(view->tail);
    return view->runStart > oldest ? view->runStart - oldest : 0;
}

static void rebuild(LogsView *view) {
    view->runStart = logsTailRunStart(view->tail);

    logsViewClear(view);
    appendFrom(view, firstVisible(view));
}

LogsView *logsViewCreate(void) {
    LogsView *view = (LogsView *)calloc(1, sizeof(LogsView));
    if (!view) return NULL;

    view->tail = logsTailCreate();
    view->editor = uiNewScintilla();
    view->minimum = LOGS_LEVEL_TRACE;
    view->autoscroll = true;
    view->currentRunOnly = true;

    configure(view->editor);
    return view;
}

uiControl *logsViewControl(LogsView *view) {
    return uiControl(view->editor);
}

void logsViewSetPath(LogsView *view, const char *path) {
    if (strcmp(logsTailPath(view->tail), path) == 0) return;

    logsTailSetPath(view->tail, path);
    rebuild(view);
}

void logsViewSetMinimumLevel(LogsView *view, LogsLevel minimum) {
    if (view->minimum == minimum) return;

    view->minimum = minimum;
    rebuild(view);
}

void logsViewSetCurrentRunOnly(LogsView *view, bool enabled) {
    if (view->currentRunOnly == enabled) return;

    view->currentRunOnly = enabled;
    rebuild(view);
}

void logsViewSetWordWrap(LogsView *view, bool enabled) {
    uiScintillaSend(view->editor, SCI_SETWRAPMODE, enabled ? SC_WRAP_WORD : SC_WRAP_NONE, 0);
}

void logsViewSetHideTimestamp(LogsView *view, bool hidden) {
    if (view->hideTimestamp == hidden) return;

    view->hideTimestamp = hidden;
    rebuild(view);
}

void logsViewSetHideOrigin(LogsView *view, bool hidden) {
    if (view->hideOrigin == hidden) return;

    view->hideOrigin = hidden;
    rebuild(view);
}

void logsViewZoomIn(LogsView *view) {
    uiScintillaSend(view->editor, SCI_ZOOMIN, 0, 0);
}

void logsViewZoomOut(LogsView *view) {
    uiScintillaSend(view->editor, SCI_ZOOMOUT, 0, 0);
}

void logsViewZoomReset(LogsView *view) {
    uiScintillaSend(view->editor, SCI_SETZOOM, 0, 0);
}

void logsViewSetAutoscroll(LogsView *view, bool enabled) {
    view->autoscroll = enabled;
    if (!enabled) return;

    uiScintillaSend(view->editor, SCI_GOTOPOS,
                    (uintptr_t)uiScintillaSend(view->editor, SCI_GETLENGTH, 0, 0), 0);
}

void logsViewRefresh(LogsView *view) {
    logsTailRead(view->tail);

    size_t total = logsTailTotal(view->tail);
    size_t count = logsTailCount(view->tail);
    if (view->currentRunOnly && logsTailRunStart(view->tail) != view->runStart) {
        rebuild(view);
        return;
    }
    if (total == view->rendered) return;

    if (view->rendered < total - count) {
        rebuild(view);
        return;
    }
    appendFrom(view, count - (total - view->rendered));
}

void logsViewClear(LogsView *view) {
    uiScintillaSend(view->editor, SCI_SETREADONLY, 0, 0);
    uiScintillaSend(view->editor, SCI_CLEARALL, 0, 0);
    uiScintillaSend(view->editor, SCI_SETREADONLY, 1, 0);
}

void logsViewCopy(LogsView *view) {
    uiScintillaSend(view->editor, SCI_COPY, 0, 0);
}

void logsViewSelectAll(LogsView *view) {
    uiScintillaSend(view->editor, SCI_SELECTALL, 0, 0);
}
