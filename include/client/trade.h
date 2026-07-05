#ifndef CLIENT_TRADE_H_
#define CLIENT_TRADE_H_

#include "client.h"

// Trade timer (mirror of service/trade.h).

typedef struct {
    bool running;
    int elapsedMs;
    int hits;
} TradeStatus;

typedef struct {
    int trades;
    int totalMs;
    int totalHits;
} TradeTotal;

ClientResult clientGetTrade(Client *client, TradeStatus *out);
ClientResult clientGetTradeTotal(Client *client, TradeTotal *out);
// Start a trade. CONFLICT/INVALID if one is already running.
ClientResult clientStartTrade(Client *client);
// End + record the current trade. out (elapsed/hits) may be NULL.
ClientResult clientEndTrade(Client *client, TradeStatus *out);
ClientResult clientCancelTrade(Client *client);

#endif // CLIENT_TRADE_H_
