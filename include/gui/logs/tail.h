#ifndef GUI_LOGS_TAIL_H_
#define GUI_LOGS_TAIL_H_

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    LOGS_LEVEL_TRACE,
    LOGS_LEVEL_DEBUG,
    LOGS_LEVEL_INFO,
    LOGS_LEVEL_WARN,
    LOGS_LEVEL_ERROR,
    LOGS_LEVEL_FATAL,
    LOGS_LEVEL_UNTAGGED
} LogsLevel;

// Byte length of the bracketed level token, e.g. "[ERROR]"
#define LOGS_LEVEL_TOKEN_SIZE 7

typedef struct {
    LogsLevel level;
    size_t tokenAt;
    size_t messageAt;
    bool header;
    char *text;
} LogsLine;

typedef struct LogsTail LogsTail;

LogsTail *logsTailCreate(void);
void logsTailDestroy(LogsTail *tail);
void logsTailSetPath(LogsTail *tail, const char *path);
const char *logsTailPath(const LogsTail *tail);
void logsTailRead(LogsTail *tail);
size_t logsTailCount(const LogsTail *tail);
size_t logsTailTotal(const LogsTail *tail);
const LogsLine *logsTailAt(const LogsTail *tail, size_t index);
size_t logsTailRunStart(const LogsTail *tail);

#endif // GUI_LOGS_TAIL_H_
