#ifndef SERVICE_GSC_H_
#define SERVICE_GSC_H_

#include <stddef.h>

#include "service.h"
#include "logic/gsc/mods.h"

#define GSC_FOLDER_SIZE 260

const GSCMod *serviceGscModList(Service *service, size_t *count);
ServiceResult serviceGscModCreate(Service *service, const char *name);
ServiceResult serviceGscRemove(Service *service, const char *path);
ServiceResult serviceGscFolder(Service *service, char *out, size_t size);
char *serviceGscScriptRead(Service *service, const char *path);
ServiceResult serviceGscScriptWrite(Service *service, const char *path, const char *content);
ServiceResult serviceGscScriptCreate(Service *service, const char *path);

#endif // SERVICE_GSC_H_
