#ifndef SERVICE_TRADE_H_
#define SERVICE_TRADE_H_

#include "service.h"

// A "trade" tracks how long the player farms the box/chest and how many hits it
// took. Timestamps come from the level elapsed time, mirroring the /trade chat command.

typedef struct {
    bool running;
    int elapsedMs;
    int hits;
} ServiceTradeStatus;

typedef struct {
    int trades;
    int totalMs;
    int totalHits;
} ServiceTradeTotal;

ServiceResult serviceTradeStatus(Service *service, ServiceTradeStatus *out);
ServiceResult serviceTradeTotal(Service *service, ServiceTradeTotal *out);
// SERVICE_INVALID_PARAM if one is already running.
ServiceResult serviceTradeStart(Service *service);
// End + record the current trade. SERVICE_INVALID_PARAM if none is running.
ServiceResult serviceTradeEnd(Service *service, ServiceTradeStatus *out);
// SERVICE_INVALID_PARAM if none is running.
ServiceResult serviceTradeCancel(Service *service);

#endif // SERVICE_TRADE_H_
