#ifndef SERVICE_CAMO_H_
#define SERVICE_CAMO_H_

#include "service.h"
#include "logic/camo/manager.h"

typedef struct {
    CamoFileType type;
    unsigned int number;
    const char *source;
} ServiceCamoFile;

typedef struct {
    bool hasName;  const char *name;
    bool hasFiles; const ServiceCamoFile *files; size_t fileCount;
} ServiceCamoPatch;

typedef struct {
    size_t camoCount;
    size_t bundleCount;
    size_t weaponCount;
    const char *activeBundleId;
} ServiceCamoOverview;

const char *serviceCamoFileTypeName(CamoFileType type);
bool serviceCamoFileTypeFromName(const char *name, CamoFileType *typeOut);

ServiceResult serviceCamoOverview(Service *service, ServiceCamoOverview *overviewOut);

const CamoWeapon *serviceCamoWeaponList(Service *service, size_t *count);
const CamoWeapon *serviceCamoWeaponFind(Service *service, const char *weaponId);
const Camo *serviceCamoList(Service *service, size_t *count);
const Camo *serviceCamoFind(Service *service, const char *camoId);
const CamoBundle *serviceCamoBundleList(Service *service, size_t *count);
const CamoBundle *serviceCamoBundleFind(Service *service, const char *bundleId);
bool serviceCamoBundleIsInstalled(Service *service, const char *bundleId);

ServiceResult serviceCamoCreate(Service *service, const char *name, const ServiceCamoFile *files, size_t fileCount, const char **idOut);
ServiceResult serviceCamoUpdate(Service *service, const char *camoId, const ServiceCamoPatch *patch);
ServiceResult serviceCamoRemove(Service *service, const char *camoId, bool removeReferences);

ServiceResult serviceCamoBundleCreate(Service *service, const char *name, const char **idOut);
ServiceResult serviceCamoBundleRename(Service *service, const char *bundleId, const char *name);
ServiceResult serviceCamoBundleRemove(Service *service, const char *bundleId);
ServiceResult serviceCamoBundleAssign(Service *service, const char *bundleId, const char *weaponId, const char *camoId);
ServiceResult serviceCamoBundleUnassign(Service *service, const char *bundleId, const char *weaponId);
ServiceResult serviceCamoBundleInstall(Service *service, const char *bundleId);
ServiceResult serviceCamoBundleUninstall(Service *service, const char *bundleId);

#endif // SERVICE_CAMO_H_
