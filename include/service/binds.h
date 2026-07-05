#ifndef SERVICE_BINDS_H_
#define SERVICE_BINDS_H_

#include "service.h"
#include "logic/config.h"

ServiceResult serviceBindsGet(Service *service, BindsConfig *configOut);
ServiceResult serviceBindsSet(Service *service, BindsConfig *config);
ServiceResult serviceBindsReset(Service *service);

#endif // SERVICE_BINDS_H_
