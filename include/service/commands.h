#ifndef SERVICE_COMMANDS_H_
#define SERVICE_COMMANDS_H_

#include "service.h"

typedef struct {
    char name[64];            // derived from usage, e.g. "god" from "/god"
    const char *usage;        // borrowed from the catalog (static strings)
    const char *description;  // borrowed from the catalog (static strings)
} ServiceCommandInfo;

ServiceResult serviceCommandsList(Service *service, ServiceCommandInfo *out, int max, int *countOut);

#endif // SERVICE_COMMANDS_H_
