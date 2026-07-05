#ifndef CLIENT_ROUND_H_
#define CLIENT_ROUND_H_

#include "client.h"

ClientResult clientGetRound(Client *client, int *round);
ClientResult clientSetRound(Client *client, int round);

// A map has at most one special type, so `type` names it ("dogs" | "monkeys" | "thief")
// and is "" when none applies; `count` is how many have happened (-1 when not
// applicable) and `next` is the upcoming-rounds prediction ("" if none).
typedef struct {
    char type[16];
    int count;
    char next[64];
} SpecialRound;

ClientResult clientGetSpecialRound(Client *client, SpecialRound *out);

#endif // CLIENT_ROUND_H_
