#ifndef TRADE_H_
#define TRADE_H_

#include <stdbool.h>

#define MAX_TRADES 512

typedef struct Trade {
    int startTimestamp;
    int endTimestamp;
    int hits;
} Trade;

void tradeClear(Trade *trade);
bool tradeStart(Trade *trade, int timestamp);
bool tradeEnd(Trade *trade, int timestamp);
bool tradeCancel(Trade *trade);
bool tradeHit(Trade *trade);
bool tradeRunning(Trade *trade);
int tradeGetElapsed(Trade *trade, int currentTimestamp);
int tradeGetHits(Trade *trade);

#endif // TRADE_H_
