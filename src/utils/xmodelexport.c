// Parses the Mod Tools source model (model_export\...\*.XMODEL_EXPORT, ASCII
// "VERSION 6") into XModelSurf mesh data for the camo preview. One XSSurface is
// emitted per OBJECT block, which keeps each attachment mesh (scope, suppressor,
// mount) on its own surface.
#include "utils/xmodelexport.h"
#include "win/file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define XE_VERSION 6

typedef struct {
    char *data;
    size_t size;
    size_t pos;
} Tokenizer;

typedef struct {
    int *remap;
    XSVertex *vertices;
    int vertexCount;
    int vertexCap;
    unsigned short *indices;
    int indexCount;
    int indexCap;
} SurfaceBuilder;

static void setErr(char *err, int errSize, const char *msg) {
    if (err && errSize > 0) {
        strncpy(err, msg, (size_t)errSize - 1);
        err[errSize - 1] = '\0';
    }
}

static bool isSeparator(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ',';
}

static bool nextToken(Tokenizer *t, char *buf, int bufSize) {
    for (;;) {
        while (t->pos < t->size && isSeparator(t->data[t->pos])) t->pos++;
        if (t->pos + 1 < t->size && t->data[t->pos] == '/' && t->data[t->pos + 1] == '/') {
            while (t->pos < t->size && t->data[t->pos] != '\n') t->pos++;
            continue;
        }
        break;
    }
    if (t->pos >= t->size) return false;

    int n = 0;
    while (t->pos < t->size && !isSeparator(t->data[t->pos])) {
        if (n < bufSize - 1) buf[n++] = t->data[t->pos];
        t->pos++;
    }
    buf[n] = '\0';
    return true;
}

static bool skipTo(Tokenizer *t, const char *keyword) {
    char tok[64];
    while (nextToken(t, tok, sizeof(tok))) {
        if (strcmp(tok, keyword) == 0) return true;
    }
    return false;
}

static bool readInt(Tokenizer *t, int *out) {
    char tok[64];
    if (!nextToken(t, tok, sizeof(tok))) return false;
    *out = atoi(tok);
    return true;
}

static bool readFloat(Tokenizer *t, float *out) {
    char tok[64];
    if (!nextToken(t, tok, sizeof(tok))) return false;
    *out = (float)atof(tok);
    return true;
}

static unsigned char toByte(float c) {
    if (c <= 0.0f) return 0;
    if (c >= 1.0f) return 255;
    return (unsigned char)(c * 255.0f + 0.5f);
}

static bool pushVertex(SurfaceBuilder *b, const XSVertex *v) {
    if (b->vertexCount >= b->vertexCap) {
        int cap = b->vertexCap ? b->vertexCap * 2 : 64;
        XSVertex *grown = (XSVertex *)realloc(b->vertices, (size_t)cap * sizeof(XSVertex));
        if (!grown) return false;
        b->vertices = grown;
        b->vertexCap = cap;
    }
    b->vertices[b->vertexCount++] = *v;
    return true;
}

static bool pushIndex(SurfaceBuilder *b, unsigned short index) {
    if (b->indexCount >= b->indexCap) {
        int cap = b->indexCap ? b->indexCap * 2 : 128;
        unsigned short *grown = (unsigned short *)realloc(b->indices, (size_t)cap * sizeof(unsigned short));
        if (!grown) return false;
        b->indices = grown;
        b->indexCap = cap;
    }
    b->indices[b->indexCount++] = index;
    return true;
}

static int resolveVertex(SurfaceBuilder *b, int globalIndex, const XSVertex *v) {
    int local = b->remap[globalIndex];
    if (local >= 0) return local;
    local = b->vertexCount;
    if (!pushVertex(b, v)) return -1;
    b->remap[globalIndex] = local;
    return local;
}

static void freeBuilders(SurfaceBuilder *builders, int count) {
    for (int i = 0; i < count; i++) {
        free(builders[i].remap);
        free(builders[i].vertices);
        free(builders[i].indices);
    }
    free(builders);
}

static bool loadFile(const char *path, char **outData, size_t *outSize, char *err, int errSize) {
    size_t size = 0;
    char *buf = fileReadAll(path, &size);
    if (!buf) {
        setErr(err, errSize, "Could not open file");
        return false;
    }
    if (size == 0) {
        setErr(err, errSize, "File too small");
        free(buf);
        return false;
    }
    *outData = buf;
    *outSize = size;
    return true;
}

bool xmodelExportLoad(const char *path, XModelSurf *out, char *err, int errSize) {
    if (!path || !out) {
        setErr(err, errSize, "Invalid arguments");
        return false;
    }
    memset(out, 0, sizeof(*out));

    char *data;
    size_t size;
    if (!loadFile(path, &data, &size, err, errSize)) return false;

    Tokenizer t = { data, size, 0 };

    int version = 0;
    if (!skipTo(&t, "VERSION") || !readInt(&t, &version)) {
        setErr(err, errSize, "Missing VERSION header");
        free(data);
        return false;
    }
    if (version != XE_VERSION) {
        setErr(err, errSize, "Unsupported XMODEL_EXPORT version");
        free(data);
        return false;
    }

    int vertCount = 0;
    if (!skipTo(&t, "NUMVERTS") || !readInt(&t, &vertCount) || vertCount <= 0) {
        setErr(err, errSize, "Missing or invalid NUMVERTS");
        free(data);
        return false;
    }

    float *positions = (float *)malloc((size_t)vertCount * 3 * sizeof(float));
    if (!positions) {
        setErr(err, errSize, "Out of memory");
        free(data);
        return false;
    }

    for (int i = 0; i < vertCount; i++) {
        int index = 0;
        if (!skipTo(&t, "VERT") || !readInt(&t, &index) || index < 0 || index >= vertCount) {
            setErr(err, errSize, "Malformed VERT block");
            free(positions);
            free(data);
            return false;
        }
        if (!skipTo(&t, "OFFSET") ||
            !readFloat(&t, &positions[index * 3 + 0]) ||
            !readFloat(&t, &positions[index * 3 + 1]) ||
            !readFloat(&t, &positions[index * 3 + 2])) {
            setErr(err, errSize, "Malformed VERT OFFSET");
            free(positions);
            free(data);
            return false;
        }
        int weightCount = 0;
        if (!skipTo(&t, "BONES") || !readInt(&t, &weightCount)) {
            setErr(err, errSize, "Malformed VERT BONES");
            free(positions);
            free(data);
            return false;
        }
        for (int w = 0; w < weightCount; w++) {
            int boneIndex;
            float weight;
            if (!skipTo(&t, "BONE") || !readInt(&t, &boneIndex) || !readFloat(&t, &weight)) {
                setErr(err, errSize, "Malformed vertex weight");
                free(positions);
                free(data);
                return false;
            }
        }
    }

    int faceCount = 0;
    if (!skipTo(&t, "NUMFACES") || !readInt(&t, &faceCount) || faceCount <= 0) {
        setErr(err, errSize, "Missing or invalid NUMFACES");
        free(positions);
        free(data);
        return false;
    }

    SurfaceBuilder *builders = NULL;
    int builderCount = 0;

    for (int f = 0; f < faceCount; f++) {
        int objectIndex, materialIndex, dummyA, dummyB;
        if (!skipTo(&t, "TRI") ||
            !readInt(&t, &objectIndex) || !readInt(&t, &materialIndex) ||
            !readInt(&t, &dummyA) || !readInt(&t, &dummyB) || objectIndex < 0) {
            setErr(err, errSize, "Malformed TRI header");
            freeBuilders(builders, builderCount);
            free(positions);
            free(data);
            return false;
        }

        if (objectIndex >= builderCount) {
            int newCount = objectIndex + 1;
            SurfaceBuilder *grown = (SurfaceBuilder *)realloc(builders, (size_t)newCount * sizeof(SurfaceBuilder));
            if (!grown) {
                setErr(err, errSize, "Out of memory");
                freeBuilders(builders, builderCount);
                free(positions);
                free(data);
                return false;
            }
            builders = grown;
            for (int b = builderCount; b < newCount; b++) {
                memset(&builders[b], 0, sizeof(SurfaceBuilder));
            }
            builderCount = newCount;
        }

        SurfaceBuilder *builder = &builders[objectIndex];
        if (!builder->remap) {
            builder->remap = (int *)malloc((size_t)vertCount * sizeof(int));
            if (!builder->remap) {
                setErr(err, errSize, "Out of memory");
                freeBuilders(builders, builderCount);
                free(positions);
                free(data);
                return false;
            }
            for (int i = 0; i < vertCount; i++) builder->remap[i] = -1;
        }

        int locals[3];
        for (int k = 0; k < 3; k++) {
            int vertIndex, uvMap;
            XSVertex v;
            float normal[3], color[4], uv[2];
            if (!skipTo(&t, "VERT") || !readInt(&t, &vertIndex) || vertIndex < 0 || vertIndex >= vertCount ||
                !skipTo(&t, "NORMAL") || !readFloat(&t, &normal[0]) || !readFloat(&t, &normal[1]) || !readFloat(&t, &normal[2]) ||
                !skipTo(&t, "COLOR") || !readFloat(&t, &color[0]) || !readFloat(&t, &color[1]) || !readFloat(&t, &color[2]) || !readFloat(&t, &color[3]) ||
                !skipTo(&t, "UV") || !readInt(&t, &uvMap) || !readFloat(&t, &uv[0]) || !readFloat(&t, &uv[1])) {
                setErr(err, errSize, "Malformed face vertex");
                freeBuilders(builders, builderCount);
                free(positions);
                free(data);
                return false;
            }
            (void)uvMap;

            v.position[0] = positions[vertIndex * 3 + 0];
            v.position[1] = positions[vertIndex * 3 + 1];
            v.position[2] = positions[vertIndex * 3 + 2];
            v.normal[0] = normal[0];
            v.normal[1] = normal[1];
            v.normal[2] = normal[2];
            v.uv[0] = uv[0];
            v.uv[1] = uv[1];
            v.color[0] = toByte(color[0]);
            v.color[1] = toByte(color[1]);
            v.color[2] = toByte(color[2]);
            v.color[3] = toByte(color[3]);

            locals[k] = resolveVertex(builder, vertIndex, &v);
            if (locals[k] < 0) {
                setErr(err, errSize, "Out of memory");
                freeBuilders(builders, builderCount);
                free(positions);
                free(data);
                return false;
            }
        }

        if (!pushIndex(builder, (unsigned short)locals[0]) ||
            !pushIndex(builder, (unsigned short)locals[2]) ||
            !pushIndex(builder, (unsigned short)locals[1])) {
            setErr(err, errSize, "Out of memory");
            freeBuilders(builders, builderCount);
            free(positions);
            free(data);
            return false;
        }
    }

    free(positions);
    free(data);

    if (builderCount <= 0) {
        setErr(err, errSize, "No surfaces in XMODEL_EXPORT");
        freeBuilders(builders, builderCount);
        return false;
    }

    out->surfaces = (XSSurface *)calloc((size_t)builderCount, sizeof(XSSurface));
    if (!out->surfaces) {
        setErr(err, errSize, "Out of memory");
        freeBuilders(builders, builderCount);
        return false;
    }
    out->surfaceCount = builderCount;
    out->version = XE_VERSION;

    for (int i = 0; i < builderCount; i++) {
        out->surfaces[i].vertices = builders[i].vertices;
        out->surfaces[i].vertexCount = builders[i].vertexCount;
        out->surfaces[i].indices = builders[i].indices;
        out->surfaces[i].indexCount = builders[i].indexCount;
        free(builders[i].remap);
    }
    free(builders);

    return true;
}

void xmodelSurfFree(XModelSurf *m) {
    if (!m) return;
    if (m->surfaces) {
        for (int i = 0; i < m->surfaceCount; i++) {
            free(m->surfaces[i].vertices);
            free(m->surfaces[i].indices);
        }
        free(m->surfaces);
    }
    m->surfaces = NULL;
    m->surfaceCount = 0;
    m->version = 0;
}
