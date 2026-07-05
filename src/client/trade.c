#include "client/trade.h"
#include "client/client_internal.h"

ClientResult clientGetTrade(Client *client, TradeStatus *out) {
    if (!out) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", CLIENT_API_BASE "/trade", NULL, &body);
    if (r != CLIENT_OK) return r;
    if (!body) return CLIENT_ERR_PROTOCOL;
    out->running = jsonObjectGetBool(body, "running", false);
    out->elapsedMs = jsonObjectGetInt(body, "elapsed-ms", 0);
    out->hits = jsonObjectGetInt(body, "hits", 0);
    jsonFree(body);
    return CLIENT_OK;
}

ClientResult clientGetTradeTotal(Client *client, TradeTotal *out) {
    if (!out) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", CLIENT_API_BASE "/trade/total", NULL, &body);
    if (r != CLIENT_OK) return r;
    if (!body) return CLIENT_ERR_PROTOCOL;
    out->trades = jsonObjectGetInt(body, "trades", 0);
    out->totalMs = jsonObjectGetInt(body, "total-ms", 0);
    out->totalHits = jsonObjectGetInt(body, "total-hits", 0);
    jsonFree(body);
    return CLIENT_OK;
}

ClientResult clientStartTrade(Client *client) {
    return clientRequest(client, "POST", CLIENT_API_BASE "/trade/start", NULL, NULL);
}

ClientResult clientEndTrade(Client *client, TradeStatus *out) {
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "POST", CLIENT_API_BASE "/trade/end", NULL, &body);
    if (r != CLIENT_OK) return r;
    if (out) {
        out->running = false;
        out->elapsedMs = body ? jsonObjectGetInt(body, "elapsed-ms", 0) : 0;
        out->hits = body ? jsonObjectGetInt(body, "hits", 0) : 0;
    }
    jsonFree(body);
    return CLIENT_OK;
}

ClientResult clientCancelTrade(Client *client) {
    return clientRequest(client, "POST", CLIENT_API_BASE "/trade/cancel", NULL, NULL);
}
