#include "win/file.h"

#include <windows.h>
#include <shlobj.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool removeTree(const char *path) {
    char pattern[MAX_PATH];
    int n = snprintf(pattern, sizeof(pattern), "%s\\*", path);
    if (n < 0 || (size_t)n >= sizeof(pattern)) {
        return RemoveDirectoryA(path);
    }

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) {
                continue;
            }
            char child[MAX_PATH];
            int cn = snprintf(child, sizeof(child), "%s\\%s", path, fd.cFileName);
            if (cn < 0 || (size_t)cn >= sizeof(child)) continue;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                removeTree(child);
            } else {
                DeleteFileA(child);
            }
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }

    return RemoveDirectoryA(path);
}

bool fileExists(const char *path) {
    if (!path || path[0] == '\0') return false;
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

bool fileCreateFolder(const char *path) {
    if (!path || path[0] == '\0') return false;

    char tmp[MAX_PATH];
    int n = snprintf(tmp, sizeof(tmp), "%s", path);
    if (n < 0 || (size_t)n >= sizeof(tmp)) return false;

    for (char *p = tmp + 1; *p; ++p) {
        if (*p == '/' || *p == '\\') {
            char sep = *p;
            *p = '\0';
            CreateDirectoryA(tmp, NULL);
            *p = sep;
        }
    }

    if (CreateDirectoryA(tmp, NULL)) return true;
    return GetLastError() == ERROR_ALREADY_EXISTS;
}

bool fileCopy(const char *src, const char *dst, bool overwrite) {
    if (!src || src[0] == '\0' || !dst || dst[0] == '\0') return false;
    return CopyFileA(src, dst, overwrite ? FALSE : TRUE);
}

bool fileMove(const char *src, const char *dst, bool overwrite) {
    if (!src || src[0] == '\0' || !dst || dst[0] == '\0') return false;
    return MoveFileExA(src, dst, overwrite ? MOVEFILE_REPLACE_EXISTING : 0);
}

bool fileDelete(const char *path) {
    if (!path || path[0] == '\0') return false;

    DWORD attrs = GetFileAttributesA(path);
    if (attrs == INVALID_FILE_ATTRIBUTES) return true;

    if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
        removeTree(path);
        return GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES;
    }

    if (DeleteFileA(path)) return true;
    DWORD err = GetLastError();
    return err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND;
}

char *fileReadAll(const char *path, size_t *outSize) {
    if (outSize) *outSize = 0;
    if (!path || path[0] == '\0') return NULL;

    FILE *f = fopen(path, "rb"); 
    if (!f) return NULL;

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long size = ftell(f);
    if (size < 0) { fclose(f); return NULL; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }

    char *buf = (char *)malloc((size_t)size + 1);
    if (!buf) { fclose(f); return NULL; }

    size_t read = fread(buf, 1, (size_t)size, f);
    fclose(f);
    buf[read] = '\0';
    if (outSize) *outSize = read;
    return buf;
}

bool fileWriteAll(const char *path, const void *data, size_t size) {
    if (!path || path[0] == '\0') return false;

    FILE *f = fopen(path, "wb");
    if (!f) return false;

    size_t written = size > 0 ? fwrite(data, 1, size, f) : 0;
    int closeErr = fclose(f);
    return written == size && closeErr == 0;
}

bool fileAppDataPath(char *out, size_t size, const char *subPath) {
    if (!out || size == 0) return false;

    char appData[MAX_PATH];
    if (FAILED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appData))) {
        return false;
    }

    int n;
    if (subPath && subPath[0] != '\0') {
        n = snprintf(out, size, "%s\\%s", appData, subPath);
    } else {
        n = snprintf(out, size, "%s", appData);
    }
    return n >= 0 && (size_t)n < size;
}
