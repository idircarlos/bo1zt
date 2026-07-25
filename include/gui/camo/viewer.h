#ifndef GUI_CAMO_VIEWER_H_
#define GUI_CAMO_VIEWER_H_

#include <stdbool.h>

typedef struct CamoViewer CamoViewer;

typedef struct {
    const char *modelPath;
    const char *colorPath;
    const char *normalPath;
    const char *specPath;
    const char *envPath;
} CamoViewerRequest;

CamoViewer *camoViewerCreate(void *hostHwnd);
void camoViewerDestroy(CamoViewer *viewer);

void camoViewerSetCamo(CamoViewer *viewer, const CamoViewerRequest *request);
void camoViewerSetAutoRotate(CamoViewer *viewer, bool enabled);

#endif // GUI_CAMO_VIEWER_H_
