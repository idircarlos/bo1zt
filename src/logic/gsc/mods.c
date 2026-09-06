#include "logic/gsc/mods.h"

#include <windows.h>

#include <stdio.h>
#include <string.h>

#include "logger.h"
#include "win/file.h"

#define GSC_MODS_PATH "gsc\\bo1zt\\mods"
#define GSC_SCRIPT_EXTENSION ".gsc"
#define GSC_MOD_ENTRY "main.gsc"
#define GSC_MOD_TEMPLATE "main() {\n}\n"

static GSCMod mods[GSC_MOD_MAX];
static size_t modCount = 0;

static bool hasScriptExtension(const char *path) {
    size_t length = strlen(path);
    size_t extension = strlen(GSC_SCRIPT_EXTENSION);
    if (length <= extension) return false;
    return _stricmp(path + length - extension, GSC_SCRIPT_EXTENSION) == 0;
}

static bool isRelativePath(const char *path) {
    if (!path || path[0] == '\0') return false;
    if (path[0] == '/' || path[0] == '\\') return false;
    if (strchr(path, ':')) return false;

    for (const char *segment = path; segment;) {
        size_t length = strcspn(segment, "/\\");
        if (length == 0) return false;
        if (length <= 2 && strncmp(segment, "..", length) == 0) return false;
        segment = segment[length] ? segment + length + 1 : NULL;
    }
    return true;
}

static bool isModPath(const char *path) {
    if (!isRelativePath(path)) return false;
    if (strlen(path) >= GSC_SCRIPT_PATH_SIZE) return false;
    return strpbrk(path, "/\\") != NULL;
}

static bool isScriptPath(const char *path) {
    return isModPath(path) && hasScriptExtension(path);
}

static bool isLeafName(const char *name) {
    if (!name || name[0] == '\0') return false;
    if (strpbrk(name, "/\\:")) return false;
    return strcmp(name, ".") != 0 && strcmp(name, "..") != 0;
}

static bool isModName(const char *name) {
    if (!isRelativePath(name)) return false;
    if (strlen(name) >= GSC_MOD_NAME_SIZE) return false;
    return strpbrk(name, "/\\") == NULL;
}

static bool modPath(const char *relative, char *out, size_t size) {
    char dir[MAX_PATH];
    if (!gscModsDir(dir, sizeof(dir))) return false;

    int n = snprintf(out, size, "%s\\%s", dir, relative);
    if (n < 0 || (size_t)n >= size) return false;

    for (char *p = out; *p; ++p) {
        if (*p == '/') *p = '\\';
    }
    return true;
}

static void addEntry(GSCMod *mod, const char *relative, bool folder) {
    if (mod->entryCount >= GSC_MOD_ENTRY_MAX) return;

    GSCEntry *entry = &mod->entries[mod->entryCount++];
    snprintf(entry->path, sizeof(entry->path), "%s", relative);
    entry->folder = folder;
}

static void scanMod(GSCMod *mod, const char *dir, const char *prefix) {
    char pattern[MAX_PATH];
    if (snprintf(pattern, sizeof(pattern), "%s\\*", dir) < 0) return;

    WIN32_FIND_DATAA found;
    HANDLE handle = FindFirstFileA(pattern, &found);
    if (handle == INVALID_HANDLE_VALUE) return;

    do {
        if (strcmp(found.cFileName, ".") == 0 || strcmp(found.cFileName, "..") == 0) continue;

        char relative[GSC_SCRIPT_PATH_SIZE];
        int n = snprintf(relative, sizeof(relative), "%s%s", prefix, found.cFileName);
        if (n < 0 || (size_t)n >= sizeof(relative)) continue;

        if (!(found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            if (hasScriptExtension(found.cFileName)) addEntry(mod, relative, false);
            continue;
        }

        addEntry(mod, relative, true);

        char child[MAX_PATH];
        char childPrefix[GSC_SCRIPT_PATH_SIZE];
        int cn = snprintf(child, sizeof(child), "%s\\%s", dir, found.cFileName);
        int pn = snprintf(childPrefix, sizeof(childPrefix), "%s/", relative);
        if (cn < 0 || (size_t)cn >= sizeof(child)) continue;
        if (pn < 0 || (size_t)pn >= sizeof(childPrefix)) continue;
        scanMod(mod, child, childPrefix);
    } while (FindNextFileA(handle, &found));

    FindClose(handle);
}

static void scanMods(const char *modsDir) {
    char pattern[MAX_PATH];
    if (snprintf(pattern, sizeof(pattern), "%s\\*", modsDir) < 0) return;

    WIN32_FIND_DATAA found;
    HANDLE handle = FindFirstFileA(pattern, &found);
    if (handle == INVALID_HANDLE_VALUE) return;

    do {
        if (!(found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
        if (!isModName(found.cFileName)) continue;
        if (modCount >= GSC_MOD_MAX) break;

        char dir[MAX_PATH];
        int n = snprintf(dir, sizeof(dir), "%s\\%s", modsDir, found.cFileName);
        if (n < 0 || (size_t)n >= sizeof(dir)) continue;

        GSCMod *mod = &mods[modCount++];
        snprintf(mod->name, sizeof(mod->name), "%s", found.cFileName);
        mod->entryCount = 0;
        scanMod(mod, dir, "");
    } while (FindNextFileA(handle, &found));

    FindClose(handle);
}

bool gscModsDir(char *out, size_t size) {
    if (fileAppFolderPath(out, size, GSC_MODS_PATH)) return true;
    LOG_ERROR("GSC Mods: failed to resolve %%APPDATA%%");
    return false;
}

void gscModsReload(void) {
    char dir[MAX_PATH];

    modCount = 0;
    if (!gscModsDir(dir, sizeof(dir))) return;
    fileCreateFolder(dir);

    scanMods(dir);
}

size_t gscModsCount(void) {
    return modCount;
}

const GSCMod *gscModsAt(size_t index) {
    if (index >= modCount) return NULL;
    return &mods[index];
}

bool gscModsCreate(const char *name) {
    if (!isModName(name)) return false;

    if (modCount >= GSC_MOD_MAX) {
        LOG_WARN("GSC Mods: mod limit of %d reached", GSC_MOD_MAX);
        return false;
    }

    char dir[MAX_PATH];
    if (!modPath(name, dir, sizeof(dir))) return false;
    if (fileExists(dir)) {
        LOG_WARN("GSC Mods: mod already exists: %s", name);
        return false;
    }
    if (!fileCreateFolder(dir)) {
        LOG_ERROR("GSC Mods: could not create %s", dir);
        return false;
    }

    char entry[MAX_PATH];
    int n = snprintf(entry, sizeof(entry), "%s\\" GSC_MOD_ENTRY, dir);
    if (n < 0 || (size_t)n >= sizeof(entry)) return false;
    if (!fileWriteAll(entry, GSC_MOD_TEMPLATE, strlen(GSC_MOD_TEMPLATE))) {
        LOG_ERROR("GSC Mods: could not write %s", entry);
        return false;
    }

    gscModsReload();
    LOG_INFO("GSC Mods: created mod %s", name);
    return true;
}

bool gscModsCreateFolder(const char *path) {
    if (!isModPath(path)) return false;

    char full[MAX_PATH];
    if (!modPath(path, full, sizeof(full))) return false;
    if (fileExists(full)) {
        LOG_WARN("GSC Mods: folder already exists: %s", path);
        return false;
    }
    if (!fileCreateFolder(full)) {
        LOG_ERROR("GSC Mods: could not create %s", full);
        return false;
    }

    gscModsReload();
    LOG_INFO("GSC Mods: created folder %s", path);
    return true;
}

char *gscModsReadScript(const char *path, size_t *outSize) {
    if (!isScriptPath(path)) return NULL;

    char full[MAX_PATH];
    if (!modPath(path, full, sizeof(full))) return NULL;
    return fileReadAll(full, outSize);
}

bool gscModsWriteScript(const char *path, const char *content, size_t size) {
    if (!isScriptPath(path) || !content || size > GSC_SCRIPT_SIZE_MAX) return false;

    char full[MAX_PATH];
    if (!modPath(path, full, sizeof(full))) return false;
    if (!fileExists(full)) return false;

    if (!fileWriteAll(full, content, size)) {
        LOG_ERROR("GSC Mods: could not write %s", full);
        return false;
    }
    return true;
}

bool gscModsCreateScript(const char *path) {
    if (!isScriptPath(path)) return false;

    char full[MAX_PATH];
    if (!modPath(path, full, sizeof(full))) return false;
    if (fileExists(full)) {
        LOG_WARN("GSC Mods: script already exists: %s", path);
        return false;
    }

    char *name = strrchr(full, '\\');
    if (!name) return false;

    *name = '\0';
    bool folderReady = fileCreateFolder(full);
    *name = '\\';
    if (!folderReady) return false;

    if (!fileWriteAll(full, "", 0)) {
        LOG_ERROR("GSC Mods: could not create %s", full);
        return false;
    }

    gscModsReload();
    LOG_INFO("GSC Mods: created script %s", path);
    return true;
}

static bool renamedPath(const char *path, const char *name, char *out, size_t size) {
    const char *leaf = strrchr(path, '/');

    int n = leaf ? snprintf(out, size, "%.*s/%s", (int)(leaf - path), path, name)
                 : snprintf(out, size, "%s", name);
    return n > 0 && (size_t)n < size;
}

bool gscModsRename(const char *path, const char *name) {
    if (!isRelativePath(path) || !isLeafName(name)) return false;

    char renamed[GSC_SCRIPT_PATH_SIZE];
    if (!renamedPath(path, name, renamed, sizeof(renamed))) return false;

    if (strpbrk(path, "/\\") == NULL) {
        if (!isModName(name)) return false;
    } else if (hasScriptExtension(path) ? !isScriptPath(renamed) : !isModPath(renamed)) {
        return false;
    }

    char from[MAX_PATH];
    char to[MAX_PATH];
    if (!modPath(path, from, sizeof(from))) return false;
    if (!modPath(renamed, to, sizeof(to))) return false;
    if (!fileExists(from)) return false;
    if (_stricmp(from, to) != 0 && fileExists(to)) {
        LOG_WARN("GSC Mods: %s already exists", renamed);
        return false;
    }

    if (!fileMove(from, to, false)) {
        LOG_ERROR("GSC Mods: could not rename %s to %s", path, renamed);
        return false;
    }

    gscModsReload();
    LOG_INFO("GSC Mods: renamed %s to %s", path, renamed);
    return true;
}

bool gscModsRemove(const char *path) {
    if (!isRelativePath(path)) return false;

    char full[MAX_PATH];
    if (!modPath(path, full, sizeof(full))) return false;
    if (!fileExists(full)) return false;

    if (!fileDelete(full)) {
        LOG_ERROR("GSC Mods: could not delete %s", full);
        return false;
    }

    gscModsReload();
    LOG_INFO("GSC Mods: removed %s", path);
    return true;
}
