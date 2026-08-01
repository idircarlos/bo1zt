#ifndef CLIENT_CAMO_H_
#define CLIENT_CAMO_H_

#include "client.h"
#include "logic/camo/manager.h" // CamoFileType

#define CLIENT_CAMO_ID_SIZE 40
#define CLIENT_CAMO_NAME_SIZE 96
#define CLIENT_CAMO_MODEL_SIZE 96
#define CLIENT_CAMO_FILE_NAME_SIZE 64

typedef struct {
    CamoFileType type;
    unsigned int number;
} ClientCamoFile;

typedef struct {
    CamoFileType type;
    unsigned int number;
    const char *source;
} ClientCamoFileSource;

typedef struct {
    CamoFileType type;
    unsigned int number;
    char fileName[CLIENT_CAMO_FILE_NAME_SIZE];
} ClientCamoWeaponFile;

typedef struct {
    char id[CLIENT_CAMO_ID_SIZE];
    char name[CLIENT_CAMO_NAME_SIZE];
    ClientCamoFile *files;
    size_t fileCount;
} ClientCamo;

typedef struct {
    char id[CLIENT_CAMO_ID_SIZE];
    char name[CLIENT_CAMO_NAME_SIZE];
    char model[CLIENT_CAMO_MODEL_SIZE];
    ClientCamoFile *files;
    size_t fileCount;
} ClientCamoWeapon;

typedef struct {
    char weaponId[CLIENT_CAMO_ID_SIZE];
    char camoId[CLIENT_CAMO_ID_SIZE];
} ClientCamoEntry;

typedef struct {
    char id[CLIENT_CAMO_ID_SIZE];
    char name[CLIENT_CAMO_NAME_SIZE];
    bool installed;
    ClientCamoEntry *entries;
    size_t entryCount;
} ClientCamoBundle;

typedef struct {
    bool hasName;  const char *name;
    bool hasFiles; const ClientCamoFileSource *files; size_t fileCount;
} ClientCamoPatch;

typedef struct {
    size_t camoCount;
    size_t bundleCount;
    size_t weaponCount;
    char activeBundleId[CLIENT_CAMO_ID_SIZE];
} ClientCamoOverview;

const char *clientCamoFileTypeName(CamoFileType type);

ClientResult clientGetCamoOverview(Client *client, ClientCamoOverview *out);

ClientResult clientGetCamoWeapons(Client *client, ClientCamoWeapon **outWeapons, size_t *outCount);
ClientResult clientGetCamoWeapon(Client *client, const char *weaponId, ClientCamoWeapon *out);
ClientResult clientGetCamoWeaponFiles(Client *client, const char *weaponId, ClientCamoWeaponFile **outFiles, size_t *outCount);
void clientFreeCamoWeaponFiles(ClientCamoWeaponFile *files);
void clientFreeCamoWeapon(ClientCamoWeapon *weapon);
void clientFreeCamoWeapons(ClientCamoWeapon *weapons, size_t count);

ClientResult clientGetCamos(Client *client, ClientCamo **outCamos, size_t *outCount);
ClientResult clientGetCamo(Client *client, const char *camoId, ClientCamo *out);
ClientResult clientCreateCamo(Client *client, const char *name, const ClientCamoFileSource *files, size_t fileCount, ClientCamo *out);
ClientResult clientUpdateCamo(Client *client, const char *camoId, const ClientCamoPatch *patch, ClientCamo *out);
ClientResult clientDeleteCamo(Client *client, const char *camoId, bool removeReferences);
void clientFreeCamo(ClientCamo *camo);
void clientFreeCamos(ClientCamo *camos, size_t count);

ClientResult clientGetCamoBundles(Client *client, ClientCamoBundle **outBundles, size_t *outCount);
ClientResult clientGetCamoBundle(Client *client, const char *bundleId, ClientCamoBundle *out);
ClientResult clientCreateCamoBundle(Client *client, const char *name, ClientCamoBundle *out);
ClientResult clientRenameCamoBundle(Client *client, const char *bundleId, const char *name, ClientCamoBundle *out);
ClientResult clientDeleteCamoBundle(Client *client, const char *bundleId);
void clientFreeCamoBundle(ClientCamoBundle *bundle);
void clientFreeCamoBundles(ClientCamoBundle *bundles, size_t count);

ClientResult clientAssignCamo(Client *client, const char *bundleId, const char *weaponId, const char *camoId);
ClientResult clientUnassignCamo(Client *client, const char *bundleId, const char *weaponId);
ClientResult clientInstallCamoBundle(Client *client, const char *bundleId);
ClientResult clientUninstallCamoBundle(Client *client, const char *bundleId);

#endif // CLIENT_CAMO_H_
