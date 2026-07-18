#ifndef CAMO_MANAGER_MANIFEST_H_
#define CAMO_MANAGER_MANIFEST_H_

#include <stdbool.h>

#include "logic/camo/manager.h"

#define CAMO_MANIFEST_VERSION 1

bool camoManifestLoad(CamoManager *manager);
void camoManifestFreeWeapons(CamoWeapon *weapons, size_t weaponCount);

#endif // CAMO_MANAGER_MANIFEST_H_
