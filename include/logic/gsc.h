#ifndef GSC_H
#define GSC_H

#include "logic/gsc/method.h"
#include "logic/server.h"

typedef struct GSC GSC;
typedef const char* GSCResponse;

typedef struct {
    int count;
    const char **args;
} GSCArgs;

typedef struct {
    GSCMethod method;
    GSCArgs *args;
} GSCCallData;

GSC* gscCreate(Server *sever);
void gscDestroy(GSC *gsc);
GSCResponse gscCall(GSC *gsc, GSCMethod method, GSCArgs args);
void gscWriteResponse(GSC *gsc, int index, GSCResponse response);
const char* gscMethodToString(GSCMethod method);
GSCArgs gscArgsCreate(int count);
void gscArgsFree(GSCArgs *gscArgs);

#endif // GSC_H
