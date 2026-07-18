#ifndef CAMO_MANAGER_IWD_H_
#define CAMO_MANAGER_IWD_H_

#include <stdbool.h>
#include <stddef.h>

#include "logic/camo/manager.h"

bool camoIwdFileName(char *out, size_t bufSize, const CamoBundle *bundle);
bool camoIwdBuild(const CamoManager *manager, const CamoBundle *bundle, const char *outPath);

#endif // CAMO_MANAGER_IWD_H_
