#include "logger.h"
#include "controller.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>

#define TIME_FORMAT "%Y-%m-%d %H:%M:%S"
#define LOG_FILE "bo1zt.log"

typedef struct {
    LogLevel level;
    int initialized;
    Controller *controller;
    FILE *logFile;
} Logger;

static Logger logger = {};

void loggerInit(Controller *controller) {
    if (logger.initialized) return;
    logger.level = L_INFO;
    logger.initialized = 1;
    logger.controller = controller;
    logger.logFile = fopen(LOG_FILE, "a");

    if (logger.logFile) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char timeBuf[20];
        strftime(timeBuf, sizeof(timeBuf), TIME_FORMAT, t);

        fprintf(logger.logFile, "=====================================\n");
        fprintf(logger.logFile, " BO1ZT Log\n");
        fprintf(logger.logFile, " Started: %s\n", timeBuf);
        fprintf(logger.logFile, "=====================================\n\n");
        fflush(logger.logFile);
    }
}

void loggerSetLevel(LogLevel level) {
    logger.level = level;
}

void loggerClose(void) {
    if (logger.logFile) {
        fclose(logger.logFile);
        logger.logFile = NULL;
    }
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
    fprintf(stderr, "\n");

    // Write to log file
    if (logger.logFile) {
        va_list args2;
        va_copy(args2, args);
        fprintf(logger.logFile, "[%s] [%s] %s:%d: ", timeBuf, levelStr, cfile, cline);
        vfprintf(logger.logFile, fmt, args2);
        fprintf(logger.logFile, "\n");
        fflush(logger.logFile);
        va_end(args2);
    }

    va_end(args);

    if (level == L_FATAL) {
        exit(EXIT_FAILURE);
    }
}
