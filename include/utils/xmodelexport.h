#ifndef UTILS_XMODELEXPORT_H_
#define UTILS_XMODELEXPORT_H_

#include <stdbool.h>

typedef struct {
    float position[3];
    float normal[3];
    float uv[2];
    unsigned char color[4];
} XSVertex;

typedef struct {
    XSVertex *vertices;
    int vertexCount;
    unsigned short *indices;
    int indexCount;
} XSSurface;

typedef struct {
    unsigned short version;
    XSSurface *surfaces;
    int surfaceCount;
} XModelSurf;

bool xmodelExportLoad(const char *path, XModelSurf *out, char *err, int errSize);

void xmodelSurfFree(XModelSurf *m);

#endif // UTILS_XMODELEXPORT_H_
