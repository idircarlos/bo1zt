#include "logger.h"
#include "../controller/controller.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#define TIME_FORMAT "%Y-%m-%d %H:%M:%S"

typedef struct {
    LogLevel level;
    int initialized;
    Controller *controller;
} Logger;

static Logger logger = {};

void loggerInit(Controller *controller) {
    if (logger.initialized) return;
    logger.level = L_INFO;
    logger.initialized = 1;
    logger.controller = controller;
}

void loggerSetLevel(LogLevel level) {
    logger.level = level;
}

void loggerLog(LogLevel level, const char *cfile, int cline, const char *fmt, ...) {
    if (!logger.initialized || level < logger.level) return;

    const char *levelStr;
    switch (level) {
        case L_TRACE: levelStr = "TRACE"; break;
        case L_DEBUG: levelStr = "DEBUG"; break;
        case L_INFO:  levelStr = "INFO "; break;
        case L_WARN:  levelStr = "WARN "; break;
        case L_ERROR: levelStr = "ERROR"; break;
        case L_FATAL: levelStr = "FATAL"; break;
        default:      levelStr = "UNKWN"; break;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timeBuf[20];
    strftime(timeBuf, sizeof(timeBuf), TIME_FORMAT, t);

    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "[%s] [%s] %s:%d: ", timeBuf, levelStr, cfile, cline);
    vfprintf(stderr, fmt, args);

    va_end(args);

    if (level == L_FATAL) {
        exit(EXIT_FAILURE);
    }
}