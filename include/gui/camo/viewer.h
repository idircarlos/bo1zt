#ifndef GUI_CAMO_VIEWER_H_
#define GUI_CAMO_VIEWER_H_

#include <stdbool.h>

typedef struct CamoViewer CamoViewer;

typedef enum {
    CAMO_VIEWER_LAYER_COLOR = 0,
    CAMO_VIEWER_LAYER_NORMAL,
    CAMO_VIEWER_LAYER_SPEC,
    CAMO_VIEWER_LAYER_ENV,
    CAMO_VIEWER_LAYER_COUNT
} CamoViewerLayer;

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
void camoViewerSetLayerEnabled(CamoViewer *viewer, CamoViewerLayer layer, bool enabled);
void camoViewerResetView(CamoViewer *viewer);

#endif // GUI_CAMO_VIEWER_H_
