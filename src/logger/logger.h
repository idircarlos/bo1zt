#ifndef LOGGER_H_
#define LOGGER_H_

#include "../controller/controller.h"

#define LOG_TRACE(fmt, ...) loggerLog(L_TRACE, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) loggerLog(L_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  loggerLog(L_INFO , __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  loggerLog(L_WARN , __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) loggerLog(L_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) loggerLog(L_FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

typedef enum {
    L_TRACE = 0,
    L_DEBUG,
    L_INFO,
    L_WARN,
    L_ERROR,
    L_FATAL
} LogLevel;

void loggerInit(Controller *controller);
void loggerSetLevel(LogLevel level);
void loggerLog(LogLevel level, const char *cfile, int cline, const char *fmt, ...);

#endif // LOGGER_H_