#ifndef CLIENT_CHEATS_H_
#define CLIENT_CHEATS_H_

#include "client.h"
#include "logic/cheat.h" // CheatName

// Resolve an API cheat name (kebab-case, e.g. "god") to its CheatName. Returns
// false for an unknown name. Lets string-driven callers (CLI, chat) reach the
// typed cheat operations below without duplicating the name mapping.
bool clientCheatFromName(const char *name, CheatName *out);

int clientCheatCount(void);
const char *clientCheatNameAt(int index); // NULL if out of range

ClientResult clientGetCheat(Client *client, CheatName cheat, bool *enabled);
ClientResult clientSetCheat(Client *client, CheatName cheat, bool enabled);

ClientResult clientGetCheats(Client *client, bool *enabledOut, int max, int *countOut);

// Batch set: apply names[i] = values[i] for i in [0, count) in one request.
// An unknown cheat fails locally.
ClientResult clientSetCheats(Client *client, const CheatName *names, const bool *values, int count);

#endif // CLIENT_CHEATS_H_
