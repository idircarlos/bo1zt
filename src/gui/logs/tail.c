#include "gui/logs/tail.h"

#include <windows.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOGS_TAIL_CAPACITY 4096
#define LOGS_TAIL_LINE_MAX 1024
#define LOGS_TAIL_BACKLOG 262144
#define LOGS_TAIL_CHUNK 8192

#define LOGS_TOKEN_LENGTH 5

#define LOGS_BANNER_MINIMUM 8
#define LOGS_TITLE_MARKER " BO1ZT "
#define LOGS_RUN_MARKER " Started: "

static const char *const LOGS_LEVEL_TOKEN[] = {
    "TRACE", "DEBUG", "INFO ", "WARN ", "ERROR", "FATAL"
};

struct LogsTail {
    char path[MAX_PATH];
    FILE *file;
    long offset;
    bool skipLine;
    char partial[LOGS_TAIL_LINE_MAX];
    size_t partialLength;
    LogsLine lines[LOGS_TAIL_CAPACITY];
    size_t head;
    size_t count;
    size_t total;
    size_t bannerAt;
    size_t runStart;
    bool afterBanner;
};

static bool startsWith(const char *text, const char *prefix, size_t length) {
    return strncmp(text, prefix, length) == 0;
}

static void parseLine(const char *line, LogsLine *out) {
    out->level = LOGS_LEVEL_UNTAGGED;
    out->tokenAt = 0;
    out->messageAt = 0;

    if (line[0] != '[') return;

    const char *close = strchr(line, ']');
    if (!close || close[1] != ' ' || close[2] != '[') return;

    const char *token = close + 2;
    for (int i = 0; i < LOGS_LEVEL_TOKEN_SIZE; i++) {
        if (token[i] == '\0') return;
    }
    if (token[LOGS_LEVEL_TOKEN_SIZE - 1] != ']') return;

    for (int level = 0; level < LOGS_LEVEL_UNTAGGED; level++) {
        if (strncmp(token + 1, LOGS_LEVEL_TOKEN[level], LOGS_TOKEN_LENGTH) != 0) continue;

        // Everything up to the "file:line: " separator belongs to the origin
        const char *origin = token + LOGS_LEVEL_TOKEN_SIZE;
        const char *separator = strstr(origin, ": ");

        out->level = (LogsLevel)level;
        out->tokenAt = (size_t)(token - line);
        out->messageAt = (size_t)((separator ? separator + 2 : origin) - line);
        return;
    }
}

static bool isBanner(const char *text, size_t length) {
    if (length < LOGS_BANNER_MINIMUM) return false;

    for (size_t i = 0; i < length; i++) {
        if (text[i] != '=') return false;
    }
    return true;
}

static void trackRun(LogsTail *tail, const char *text, size_t length) {
    if (isBanner(text, length)) tail->bannerAt = tail->total;
    else if (startsWith(text, LOGS_RUN_MARKER, sizeof(LOGS_RUN_MARKER) - 1)) tail->runStart = tail->bannerAt;
}

static bool isHeader(LogsTail *tail, const char *text, size_t length) {
    bool banner = isBanner(text, length);
    bool blankAfterBanner = tail->afterBanner && length == 0;
    tail->afterBanner = banner;

    return banner || blankAfterBanner
        || startsWith(text, LOGS_TITLE_MARKER, sizeof(LOGS_TITLE_MARKER) - 1)
        || startsWith(text, LOGS_RUN_MARKER, sizeof(LOGS_RUN_MARKER) - 1);
}

static void pushLine(LogsTail *tail, const char *text, size_t length) {
    char *copy = (char *)malloc(length + 1);
    if (!copy) return;
    memcpy(copy, text, length);
    copy[length] = '\0';

    trackRun(tail, copy, length);
    bool header = isHeader(tail, copy, length);

    size_t slot = (tail->head + tail->count) % LOGS_TAIL_CAPACITY;
    if (tail->count == LOGS_TAIL_CAPACITY) {
        free(tail->lines[tail->head].text);
        tail->head = (tail->head + 1) % LOGS_TAIL_CAPACITY;
    } else {
        tail->count++;
    }

    parseLine(copy, &tail->lines[slot]);
    tail->lines[slot].header = header;
    tail->lines[slot].text = copy;
    tail->total++;
}

static void feed(LogsTail *tail, const char *data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        char c = data[i];
        if (c == '\r') continue;

        if (c != '\n') {
            if (tail->partialLength < LOGS_TAIL_LINE_MAX - 1) tail->partial[tail->partialLength++] = c;
            continue;
        }

        if (tail->skipLine) tail->skipLine = false;
        else pushLine(tail, tail->partial, tail->partialLength);
        tail->partialLength = 0;
    }
}

static void forget(LogsTail *tail) {
    for (size_t i = 0; i < tail->count; i++) {
        free(tail->lines[(tail->head + i) % LOGS_TAIL_CAPACITY].text);
    }
    tail->head = 0;
    tail->count = 0;
    tail->total = 0;
    tail->offset = 0;
    tail->partialLength = 0;
    tail->skipLine = false;
    tail->bannerAt = 0;
    tail->runStart = 0;
    tail->afterBanner = false;
}

static bool openFile(LogsTail *tail) {
    if (tail->path[0] == '\0') return false;

    tail->file = fopen(tail->path, "rb");
    if (!tail->file) return false;

    if (fseek(tail->file, 0, SEEK_END) == 0) {
        long size = ftell(tail->file);
        if (size > LOGS_TAIL_BACKLOG) {
            tail->offset = size - LOGS_TAIL_BACKLOG;
            tail->skipLine = true;
        }
    }
    return true;
}

LogsTail *logsTailCreate(void) {
    return (LogsTail *)calloc(1, sizeof(LogsTail));
}

void logsTailDestroy(LogsTail *tail) {
    if (!tail) return;

    forget(tail);
    if (tail->file) fclose(tail->file);
    free(tail);
}

void logsTailSetPath(LogsTail *tail, const char *path) {
    if (strcmp(tail->path, path) == 0) return;

    if (tail->file) {
        fclose(tail->file);
        tail->file = NULL;
    }
    forget(tail);
    snprintf(tail->path, sizeof(tail->path), "%s", path);
}

const char *logsTailPath(const LogsTail *tail) {
    return tail->path;
}

void logsTailRead(LogsTail *tail) {
    if (!tail->file && !openFile(tail)) return;

    clearerr(tail->file);
    if (fseek(tail->file, 0, SEEK_END) != 0) return;

    long size = ftell(tail->file);
    if (size < 0) return;

    if (size < tail->offset) {
        tail->offset = 0;
        tail->partialLength = 0;
    }
    if (size == tail->offset) return;
    if (fseek(tail->file, tail->offset, SEEK_SET) != 0) return;

    char chunk[LOGS_TAIL_CHUNK];
    size_t read;
    while ((read = fread(chunk, 1, sizeof(chunk), tail->file)) > 0) feed(tail, chunk, read);

    long position = ftell(tail->file);
    tail->offset = position < 0 ? size : position;
}

size_t logsTailCount(const LogsTail *tail) {
    return tail->count;
}

size_t logsTailTotal(const LogsTail *tail) {
    return tail->total;
}

const LogsLine *logsTailAt(const LogsTail *tail, size_t index) {
    return &tail->lines[(tail->head + index) % LOGS_TAIL_CAPACITY];
}

size_t logsTailRunStart(const LogsTail *tail) {
    return tail->runStart;
}
