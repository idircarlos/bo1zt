#ifndef CLIENT_GSC_H_
#define CLIENT_GSC_H_

#include "client.h"

#define CLIENT_GSC_NAME_SIZE 64
#define CLIENT_GSC_PATH_SIZE 128
#define CLIENT_GSC_FOLDER_SIZE 260

typedef struct {
    char path[CLIENT_GSC_PATH_SIZE];
} ClientGscFile;

typedef struct {
    char name[CLIENT_GSC_NAME_SIZE];
    ClientGscFile *files;
    size_t fileCount;
} ClientGscMod;

typedef struct {
    char folder[CLIENT_GSC_FOLDER_SIZE];
    ClientGscMod *mods;
    size_t modCount;
} ClientGscMods;

ClientResult clientGetGscMods(Client *client, ClientGscMods *out);
void clientFreeGscMods(ClientGscMods *mods);
ClientResult clientCreateGscMod(Client *client, const char *name);
ClientResult clientDeleteGscPath(Client *client, const char *path);
ClientResult clientReadGscScript(Client *client, const char *path, char **out);
ClientResult clientWriteGscScript(Client *client, const char *path, const char *content);
ClientResult clientCreateGscScript(Client *client, const char *path);

#endif // CLIENT_GSC_H_
