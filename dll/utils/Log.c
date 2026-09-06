#include "Log.h"
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#define TIME_FORMAT "%Y-%m-%d %H:%M:%S"
#define LOG_FOLDER "bo1zt\\logs"

typedef struct {
    LogLevel level;
    bool initialized;
    char logFilePath[MAX_PATH];
} Logger;

static Logger logger = {0};

static void GetTimestamp(char* buffer, size_t bufferSize) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    _snprintf(buffer, bufferSize - 1,
             "%04d-%02d-%02d %02d:%02d:%02d",
             st.wYear, st.wMonth, st.wDay,
             st.wHour, st.wMinute, st.wSecond);
    buffer[bufferSize - 1] = '\0';
}

static const char* LogGetLevelString(LogLevel level) {
    switch (level)
    {
        case LOG_TRACE: return "TRACE";
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO:  return "INFO ";
        case LOG_WARN:  return "WARN ";
        case LOG_ERROR: return "ERROR";
        case LOG_FATAL: return "FATAL";
        default:        return "UNKWN";
    }
}

static void CreateDirs(char* path) {
    for (char* p = path + 1; *p; ++p) {
        if (*p != '\\') continue;
        *p = '\0';
        CreateDirectoryA(path, NULL);
        *p = '\\';
    }
    CreateDirectoryA(path, NULL);
}

static bool LogFolder(char* out, size_t size) {
    char appData[MAX_PATH];
    if (!GetEnvironmentVariableA("APPDATA", appData, sizeof(appData))) return false;

    int written = _snprintf(out, size, "%s\\%s", appData, LOG_FOLDER);
    return written > 0 && (size_t)written < size;
}

bool LogInit(const char* filename) {
    if (logger.initialized) return true;
    if (!filename) return false;

    char folder[MAX_PATH];
    if (!LogFolder(folder, sizeof(folder))) return false;
    CreateDirs(folder);

    _snprintf(logger.logFilePath, MAX_PATH - 1, "%s\\%s", folder, filename);
    logger.logFilePath[MAX_PATH - 1] = '\0';
    
    // Open log file in append mode to preserve previous sessions
    FILE* file = fopen(logger.logFilePath, "a");
    if (!file)
        return false;
    
    // Write header for this session
    char timestamp[64];
    GetTimestamp(timestamp, sizeof(timestamp));
    fprintf(file, "=====================================\n");
    fprintf(file, " BO1ZT DLL Log\n");
    fprintf(file, " Started: %s\n", timestamp);
    fprintf(file, "=====================================\n\n");
    
    fclose(file);
    
    logger.level = LOG_INFO;
    logger.initialized = true;
    return true;
}

bool LogDirectory(char* out, size_t size) {
    if (!logger.initialized || !out) return false;

    _snprintf(out, size - 1, "%s", logger.logFilePath);
    out[size - 1] = '\0';

    char* lastSlash = strrchr(out, '\\');
    if (!lastSlash) return false;

    *lastSlash = '\0';
    return true;
}

void LogSetLevel(LogLevel level) {    logger.level = level;
}

void LogWrite(LogLevel level, const char* file, int line, const char* format, ...) {
    if (!logger.initialized || !format)
        return;
    
    if (level < logger.level)
        return;
    
    // Get timestamp
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), TIME_FORMAT, t);
    
    // Extract just the filename from the full path
    const char* filename = strrchr(file, '\\');
    if (filename) filename++; // Skip the backslash
    else filename = file; // No path separator found, use as-is
    
    // Open file in append mode
    FILE* file_handle = fopen(logger.logFilePath, "a");
    if (!file_handle) return;
    
    // Write timestamp, level, file and line
    fprintf(file_handle, "[%s] [%s] %s:%d: ", timestamp, LogGetLevelString(level), filename, line);
    
    // Write the formatted message
    va_list args;
    va_start(args, format);
    vfprintf(file_handle, format, args);
    va_end(args);
    
    // Add newline
    fprintf(file_handle, "\n");
    
    // Close immediately for real-time viewing
    fclose(file_handle);
}
