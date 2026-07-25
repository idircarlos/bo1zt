#ifndef CAMO_MANAGER_H_
#define CAMO_MANAGER_H_

#include <stdbool.h>
#include <stddef.h>

typedef struct CamoManager CamoManager;

#define CAMO_FILE_TYPE_COUNT 4

typedef enum {
    CAMO_FILE_SPEC,
    CAMO_FILE_COLOR,
    CAMO_FILE_ENV,
    CAMO_FILE_NORMAL
} CamoFileType;

typedef struct {
    CamoFileType type;
    unsigned int number;
    char *fileName;
} CamoFile;

typedef struct {
    char *id;
    char *name;
    CamoFile *files;
    size_t fileCount;
} Camo;

typedef struct {
    char *weaponId;
    char *camoId;
} CamoBundleEntry;

typedef struct {
    char *id;
    char *name;
    CamoBundleEntry *entries;
    size_t entryCount;
} CamoBundle;

typedef struct {
    char *id;
    char *name;
    char *model;
    CamoFile *files;
    size_t fileCount;
} CamoWeapon;

typedef enum {
    CAMO_RESULT_OK,
    CAMO_RESULT_INVALID,
    CAMO_RESULT_NOT_FOUND,
    CAMO_RESULT_IN_USE
} CamoResult;

CamoManager *camoManagerCreate(void);
void camoManagerDestroy(CamoManager *manager);

CamoResult camoManagerCamoCreate(CamoManager *, const char *name, const CamoFile *files, size_t fileCount);
CamoResult camoManagerCamoUpdate(CamoManager *, const char *camoId, const char *name, const CamoFile *files, size_t fileCount);
CamoResult camoManagerCamoRemove(CamoManager *, const char *camoId, bool removeReferences);

CamoResult camoManagerBundleCreate(CamoManager *, const char *name);
CamoResult camoManagerBundleUpdate(CamoManager *, const char *bundleId, const char *name);
CamoResult camoManagerBundleRemove(CamoManager *, const char *bundleId);
CamoResult camoManagerBundleAddCamo(CamoManager *, const char *bundleId, const char *weaponId, const char *camoId);
CamoResult camoManagerBundleRemoveCamo(CamoManager *, const char *bundleId, const char *weaponId);
CamoResult camoManagerBundleInstall(CamoManager *, const char *bundleId, const char *gameLocation);
CamoResult camoManagerBundleUninstall(CamoManager *, const char *gameLocation);

const Camo *camoManagerGetCamos(const CamoManager *manager, size_t *count);
const CamoBundle *camoManagerGetBundles(const CamoManager *manager, size_t *count);
const CamoWeapon *camoManagerGetWeapons(const CamoManager *manager, size_t *count);
const char *camoManagerGetActiveBundleId(const CamoManager *manager);
const char *camoManagerGetInstalledBundleFile(const CamoManager *manager);

bool camoManagerCamoFilePath(const CamoManager *manager, const char *camoId, CamoFileType type, unsigned int number, char *out, size_t size);
bool camoManagerWeaponModelPath(const CamoManager *manager, const char *weaponId, char *out, size_t size);

#endif // CAMO_MANAGER_H_
