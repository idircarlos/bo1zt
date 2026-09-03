#ifndef GUI_LOGS_VIEW_H_
#define GUI_LOGS_VIEW_H_

#include <stdbool.h>
#include <ui.h>

#include "gui/logs/tail.h"

typedef struct LogsView LogsView;

LogsView *logsViewCreate(void);
uiControl *logsViewControl(LogsView *view);
void logsViewSetPath(LogsView *view, const char *path);
void logsViewSetMinimumLevel(LogsView *view, LogsLevel minimum);
void logsViewSetAutoscroll(LogsView *view, bool enabled);
void logsViewSetCurrentRunOnly(LogsView *view, bool enabled);
void logsViewSetWordWrap(LogsView *view, bool enabled);
void logsViewSetHideTimestamp(LogsView *view, bool hidden);
void logsViewSetHideOrigin(LogsView *view, bool hidden);
void logsViewZoomIn(LogsView *view);
void logsViewZoomOut(LogsView *view);
void logsViewZoomReset(LogsView *view);
void logsViewRefresh(LogsView *view);
void logsViewClear(LogsView *view);
void logsViewCopy(LogsView *view);
void logsViewSelectAll(LogsView *view);

#endif // GUI_LOGS_VIEW_H_
