#ifndef SERVICE_SERVER_H_
#define SERVICE_SERVER_H_

#include "service.h"

ServiceResult serviceServerCommand(Service *service, const char *command);
// On success, *valueOut is a heap string the caller must free.
ServiceResult serviceServerGetDvar(Service *service, const char *name, char **valueOut);
ServiceResult serviceServerSetDvar(Service *service, const char *name, const char *value);

#endif // SERVICE_SERVER_H_
