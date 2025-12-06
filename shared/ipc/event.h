#ifndef SHARED_EVENT_H_
#define SHARED_EVENT_H_

#define EVENT_MAP_NAME_MAX_SIZE 64
#define EVENT_MESSAGE_MAX_SIZE 512
#define EVENT_NAME_MAX_SIZE 64

typedef enum {
    EVENT_INVALID = -1,
    EVENT_CHAT_MESSAGE,
    EVENT_MAP_CHANGE,
    EVENT_MAP_RESTART,
    EVENT_VM_NOTIFY,
    EVENT_ID_UPDATE
} EventType;

typedef struct {
    int clientId;
    char message[EVENT_MESSAGE_MAX_SIZE];
} ChatMessageEventData;

typedef struct {
    char mapName[EVENT_MAP_NAME_MAX_SIZE];
} MapChangeEventData;

typedef struct {
    char _unused;   // Marker
} MapRestartEventData;

typedef struct {
    char eventName[EVENT_NAME_MAX_SIZE];
    int eventValue;
} VMNotifyEventData;

typedef struct {
    unsigned int eventId;
    int *pEventValue;
} IDUpdateEventData;

typedef union {
    ChatMessageEventData chat;
    MapChangeEventData mapChange;
    MapRestartEventData mapRestart;
    VMNotifyEventData vmNotify;
    IDUpdateEventData idUpdate;
} EventData;

typedef struct {
    EventType type;
    EventData data;
    unsigned int timestamp;
} Event;

#endif // SHARED_EVENT_H_
