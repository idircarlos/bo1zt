#ifndef SERVICE_CHEATS_H_
#define SERVICE_CHEATS_H_

#include "service.h"

int serviceCheatCount(void);
const char *serviceCheatNameAt(int index);
bool serviceCheatExists(const char *name);

ServiceResult serviceCheatGet(Service *service, const char *name, bool *enabledOut);
ServiceResult serviceCheatSet(Service *service, const char *name, bool enabled, bool *enabledOut);

#endif // SERVICE_CHEATS_H_
