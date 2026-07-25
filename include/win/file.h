#ifndef FILE_H_
#define FILE_H_

#include <stddef.h>
#include <stdbool.h>

bool fileExists(const char *path);
bool fileCreateFolder(const char *path);
bool fileCopy(const char *src, const char *dst, bool overwrite);
bool fileMove(const char *src, const char *dst, bool overwrite);
bool fileDelete(const char *path);
char *fileReadAll(const char *path, size_t *outSize);
bool fileWriteAll(const char *path, const void *data, size_t size);
bool fileAppDataPath(char *out, size_t size, const char *subPath);

#endif // FILE_H_
