#ifndef GSC_MODS_H
#define GSC_MODS_H

#include <stdbool.h>
#include <stddef.h>

#define GSC_MOD_NAME_SIZE 64
#define GSC_SCRIPT_PATH_SIZE 128
#define GSC_MOD_MAX 32
#define GSC_MOD_FILE_MAX 32
#define GSC_SCRIPT_SIZE_MAX (256 * 1024)

typedef struct {
    char path[GSC_SCRIPT_PATH_SIZE];
} GSCScript;

typedef struct {
    char name[GSC_MOD_NAME_SIZE];
    GSCScript files[GSC_MOD_FILE_MAX];
    size_t fileCount;
} GSCMod;

bool gscModsDir(char *out, size_t size);
void gscModsReload(void);
size_t gscModsCount(void);
const GSCMod *gscModsAt(size_t index);
bool gscModsCreate(const char *name);
bool gscModsRemove(const char *path);
char *gscModsReadScript(const char *path, size_t *outSize);
bool gscModsWriteScript(const char *path, const char *content, size_t size);
bool gscModsCreateScript(const char *path);

#endif // GSC_MODS_H
