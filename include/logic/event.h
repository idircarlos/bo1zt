#ifndef EVENT_H_
#define EVENT_H_

#include "controller.h"
#include "ipc/event.h"

void eventInit(Controller *controller);
Event eventPoll();
bool eventHandle(Event event);

#endif // EVENT_H_
