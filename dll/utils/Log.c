#include "Log.h"
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#define TIME_FORMAT "%Y-%m-%d %H:%M:%S"

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

// Store DLL module handle for getting its path
static HMODULE dllModule = NULL;

void LogSetModule(HMODULE hModule) {
    dllModule = hModule;
}

bool LogInit(const char* filename) {
    if (logger.initialized) return true;
    if (!filename) return false;
    
    // Get the DLL's directory (where bo1zt.dll is located)
    char modulePath[MAX_PATH];
    GetModuleFileNameA(dllModule, modulePath, MAX_PATH);
    
    // Extract directory from full path
    char* lastSlash = strrchr(modulePath, '\\');
    if (lastSlash)
        *lastSlash = '\0';
    
    // Build full log file path in the bo1zt folder
    _snprintf(logger.logFilePath, MAX_PATH - 1, "%s\\%s", modulePath, filename);
    logger.logFilePath[MAX_PATH - 1] = '\0';
    
    // Create/truncate the log file
    FILE* file = fopen(logger.logFilePath, "w");
    if (!file)
        return false;
    
    // Write header
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

void LogSetLevel(LogLevel level) {
    logger.level = level;
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
