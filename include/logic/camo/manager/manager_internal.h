#ifndef CAMO_MANAGER_INTERNAL_H_
#define CAMO_MANAGER_INTERNAL_H_

#include <stddef.h>

#include "logic/camo/manager.h"

struct CamoManager {
    Camo *camos;
    size_t camoCount;
    CamoBundle *bundles;
    size_t bundleCount;
    CamoWeapon *weapons;
    size_t weaponCount;
    char *activeBundleId;
    char *installedBundleFile;
};

#endif // CAMO_MANAGER_INTERNAL_H_
