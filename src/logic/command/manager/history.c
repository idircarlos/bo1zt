#include "logic/command/manager/history.h"
#include "utils/list.h"
#include <stdlib.h>

struct History {
    List *entries;
    int cursor;
};

static Command *historyGetAt(History *history, int index);

History *historyCreate(void) {
    History *h = (History *)malloc(sizeof(History));
    if (!h) return NULL;
    
    h->entries = listCreate();
    h->cursor = -1;
    return h;
}

void historyDestroy(History *history) {
    if (!history) return;
    
    size_t count = listSize(history->entries);
    for (size_t i = 0; i < count; i++) {
        commandFree((Command *)listGet(history->entries, i));
    }
    listDestroy(history->entries);
    free(history);
}

void historyAdd(History *history, const Command *command) {
    if (!history || !command) return;
    
    Command *copy = commandCopy(command);
    if (!copy) return;
    
    listAdd(history->entries, copy);
    history->cursor = (int)listSize(history->entries);
}

void historyReset(History *history) {
    if (!history) return;
    history->cursor = (int)listSize(history->entries);
}

Command *historyGetPrevious(History *history) {
    if (!history) return NULL;
    if (history->cursor > 0) history->cursor--;
    return historyGetAt(history, history->cursor);
}

Command *historyGetNext(History *history) {
    if (!history) return NULL;
    int count = (int)listSize(history->entries);
    if (history->cursor < count) history->cursor++;
    return historyGetAt(history, history->cursor);
}

static Command *historyGetAt(History *history, int index) {
    int count = (int)listSize(history->entries);
    if (index < 0 || index >= count) return NULL;
    return (Command *)listGet(history->entries, index);
}
