#ifndef SHARED_EVENT_H_
#define SHARED_EVENT_H_

#define EVENT_DATA_MAX_SIZE 1024

typedef enum {
    EVENT_INVALID = -1,
    EVENT_CHAT_MESSAGE,
    EVENT_MAP_CHANGE,
    EVENT_MAP_RESTART,
    EVENT_VM_NOTIFY,
    EVENT_ID_UPDATE
} EventType;

typedef struct {
    EventType type;
    char data[EVENT_DATA_MAX_SIZE];
} Event;

#endif // SHARED_EVENT_H_
