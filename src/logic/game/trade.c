#include "logic/game/trade.h"

void tradeClear(Trade *trade) {
    trade->startTimestamp = 0;
    trade->endTimestamp = 0;
    trade->hits = 0;
}

bool tradeStart(Trade *trade, int timestamp) {
    if (tradeRunning(trade)) return false;
    trade->startTimestamp = timestamp;
    trade->endTimestamp = 0;
    trade->hits = 0;
    return true;
}

bool tradeEnd(Trade *trade, int timestamp) {
    if (!tradeRunning(trade)) return false;
    trade->endTimestamp = timestamp;
    return true;
}

bool tradeCancel(Trade *trade) {
    if (!tradeRunning(trade)) return false;
    tradeClear(trade);
    return true;
}

bool tradeHit(Trade *trade) {
    if (!tradeRunning(trade)) return false;
    trade->hits++;
    return true;
}

bool tradeRunning(Trade *trade) {
    return trade->startTimestamp != 0 && trade->endTimestamp == 0;
}

int tradeGetElapsed(Trade *trade, int currentTimestamp) {
    if (trade->endTimestamp != 0) {
        return trade->endTimestamp - trade->startTimestamp;
    }
    return currentTimestamp - trade->startTimestamp;
}

int tradeGetHits(Trade *trade) {
    return trade->hits;
}
