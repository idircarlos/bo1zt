#ifndef DLL_LOG_H
#define DLL_LOG_H

#include <stdbool.h>

typedef enum {
    LOG_TRACE = 0,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
} LogLevel;

bool LogInit(const char* filename);
void LogSetLevel(LogLevel level);
void LogWrite(LogLevel level, const char* file, int line, const char* format, ...);

#define LOG_TRACE(fmt, ...) LogWrite(LOG_TRACE, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) LogWrite(LOG_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  LogWrite(LOG_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  LogWrite(LOG_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) LogWrite(LOG_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) LogWrite(LOG_FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif // DLL_LOG_H
