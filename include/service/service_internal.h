#ifndef SERVICE_INTERNAL_H_
#define SERVICE_INTERNAL_H_

#include "service.h"
#include "controller.h"

struct Service {
    Controller *controller;
};

#endif // SERVICE_INTERNAL_H_
