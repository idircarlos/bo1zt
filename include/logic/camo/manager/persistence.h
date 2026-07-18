#ifndef CAMO_MANAGER_PERSISTENCE_H_
#define CAMO_MANAGER_PERSISTENCE_H_

#include <stdbool.h>
#include <stddef.h>

#include "logic/camo/manager.h"

#define CAMO_SCHEMA_VERSION 1

bool camoPersistenceValidId(const char *id);
bool camoPersistenceCamoDir(char *out, size_t bufSize, const char *camoId);
bool camoPersistenceBundleDir(char *out, size_t bufSize, const char *bundleId);
bool camoPersistenceCamoFilePath(char *out, size_t bufSize, const char *camoId, CamoFileType type, unsigned int number);
bool camoPersistenceCopy(const char *src, const char *dst, bool replace);
bool camoPersistenceDelete(const char *path);
bool camoPersistenceImportCamoFiles(const char *camoId, const CamoFile *files, size_t fileCount);
bool camoPersistenceParseFileKey(const char *key, CamoFileType *type, unsigned int *number);
bool camoPersistenceLoad(CamoManager *manager);
bool camoPersistenceSave(const CamoManager *manager);

#endif // CAMO_MANAGER_PERSISTENCE_H_
