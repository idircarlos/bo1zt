#ifndef SERVICE_ROUND_H_
#define SERVICE_ROUND_H_

#include "service.h"

typedef struct {
    int number;
    bool isSpecial;
    int zombiesLeft;
} ServiceRoundInfo;

// A map has at most one special-round type, so `type` names it
// ("dogs" | "monkeys" | "thief") and is "" when the map has none.
// `count` is how many have occurred so far (-1 when not applicable) and
// `next` is a human-readable prediction of upcoming special-round numbers.
typedef struct {
    char type[16];
    int count;
    char next[64];
} ServiceSpecialRound;

ServiceResult serviceRoundGet(Service *service, ServiceRoundInfo *infoOut);
ServiceResult serviceRoundSet(Service *service, int round);
ServiceResult serviceRoundGetSpecial(Service *service, ServiceSpecialRound *out);

#endif // SERVICE_ROUND_H_
