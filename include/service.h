#ifndef SERVICE_H_
#define SERVICE_H_

#include <stdbool.h>
#include "controller.h"

#define SERVICE_DEFAULT_PORT 27815

typedef struct Service Service;

typedef enum {
    SERVICE_OK = 0,
    SERVICE_NOT_FOUND,          // unknown resource (e.g. unknown cheat name)
    SERVICE_INVALID_PARAM,      // malformed/missing parameter
    SERVICE_GAME_NOT_ATTACHED,  // game process is not attached
    SERVICE_ENGINE_FAILED,      // underlying engine/api call failed
} ServiceResult;

Service *serviceCreate(Controller *controller);
void serviceDestroy(Service *service);

int serviceResolvePort(void);
void serviceServe(Service *service, int port);

const char *serviceGetVersion(Service *service);

#endif // SERVICE_H_
