#ifndef COMMAND_HISTORY_H_
#define COMMAND_HISTORY_H_

#include "logic/command.h"

typedef struct History History;

History *historyCreate(void);
void historyDestroy(History *history);
void historyAdd(History *history, const Command *command);
void historyReset(History *history);
Command *historyGetPrevious(History *history);
Command *historyGetNext(History *history);

#endif // COMMAND_HISTORY_H_
