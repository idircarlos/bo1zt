#include "gui/camo/viewer.h"
#include "utils/xmodelexport.h"
#include "utils/iwi.h"
#include "win/resources.h"
#include "resource_ids.h"
#include "logger.h"

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <GL/gl.h>
#include <float.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#define VIEWER_FOV_DEG 45.0f
#define VIEWER_FIT_MARGIN 1.2f
#define VIEWER_AUTOROTATE_DEG_PER_SEC 100.0f
#define VIEWER_TIMER_ID 1
#define VIEWER_SUBCLASS_ID 1
#define VIEWER_FALLBACK_REFRESH_HZ 60
#define VIEWER_INITIAL_PITCH_DEG 15.0f
#define VIEWER_INITIAL_YAW_DEG 30.0f
#define VIEWER_PITCH_LIMIT_DEG 89.0f
#define VIEWER_DRAG_DEG_PER_PIXEL 0.5f
#define VIEWER_ZOOM_PER_NOTCH 0.9f
#define VIEWER_MIN_DISTANCE_FACTOR 0.2f
#define VIEWER_MAX_DISTANCE_FACTOR 6.0f
#define VIEWER_MAX_FRAME_SECONDS 0.1
#define VIEWER_PI 3.14159265f

#define degToRad(deg) ((deg) * VIEWER_PI / 180.0f)

typedef enum {
    VIEWER_LAYER_COLOR = 0,
    VIEWER_LAYER_NORMAL,
    VIEWER_LAYER_SPEC,
    VIEWER_LAYER_ENV,
    VIEWER_LAYER_COUNT
} ViewerLayer;

static const char *LAYER_SAMPLER_UNIFORM[VIEWER_LAYER_COUNT] = {
    "colorMap", "normalMap", "specMap", "envMap"
};

static const char *LAYER_ENABLED_UNIFORM[VIEWER_LAYER_COUNT] = {
    "hasColor", "hasNormal", "hasSpec", "hasEnv"
};

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER   0x8B31
#define GL_COMPILE_STATUS  0x8B81
#define GL_LINK_STATUS     0x8B82
#define GL_TEXTURE0        0x84C0

typedef char GLchar;

typedef GLuint (APIENTRY *PFNglCreateShader)(GLenum);
typedef void   (APIENTRY *PFNglShaderSource)(GLuint, GLsizei, const GLchar* const*, const GLint*);
typedef void   (APIENTRY *PFNglCompileShader)(GLuint);
typedef void   (APIENTRY *PFNglGetShaderiv)(GLuint, GLenum, GLint*);
typedef void   (APIENTRY *PFNglGetShaderInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef GLuint (APIENTRY *PFNglCreateProgram)(void);
typedef void   (APIENTRY *PFNglAttachShader)(GLuint, GLuint);
typedef void   (APIENTRY *PFNglLinkProgram)(GLuint);
typedef void   (APIENTRY *PFNglGetProgramiv)(GLuint, GLenum, GLint*);
typedef void   (APIENTRY *PFNglGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void   (APIENTRY *PFNglUseProgram)(GLuint);
typedef void   (APIENTRY *PFNglDeleteShader)(GLuint);
typedef void   (APIENTRY *PFNglDeleteProgram)(GLuint);
typedef GLint  (APIENTRY *PFNglGetUniformLocation)(GLuint, const GLchar*);
typedef void   (APIENTRY *PFNglUniform1i)(GLint, GLint);
typedef void   (APIENTRY *PFNglUniform1f)(GLint, GLfloat);
typedef void   (APIENTRY *PFNglUniform3f)(GLint, GLfloat, GLfloat, GLfloat);
typedef void   (APIENTRY *PFNglActiveTexture)(GLenum);

typedef struct {
    PFNglCreateShader createShader;
    PFNglShaderSource shaderSource;
    PFNglCompileShader compileShader;
    PFNglGetShaderiv getShaderiv;
    PFNglGetShaderInfoLog getShaderInfoLog;
    PFNglCreateProgram createProgram;
    PFNglAttachShader attachShader;
    PFNglLinkProgram linkProgram;
    PFNglGetProgramiv getProgramiv;
    PFNglGetProgramInfoLog getProgramInfoLog;
    PFNglUseProgram useProgram;
    PFNglDeleteShader deleteShader;
    PFNglDeleteProgram deleteProgram;
    PFNglGetUniformLocation getUniformLocation;
    PFNglUniform1i uniform1i;
    PFNglUniform1f uniform1f;
    PFNglUniform3f uniform3f;
    PFNglActiveTexture activeTexture;
} GLShaderApi;

static GLShaderApi gl;

static bool loadShaderApi(void) {
    #define LOAD(field, type, name) do { \
        gl.field = (type)(void (*)(void))wglGetProcAddress(name); \
        if (!gl.field) { LOG_ERROR("Camo viewer: missing GL function %s", name); return false; } \
    } while (0)

    LOAD(createShader, PFNglCreateShader, "glCreateShader");
    LOAD(shaderSource, PFNglShaderSource, "glShaderSource");
    LOAD(compileShader, PFNglCompileShader, "glCompileShader");
    LOAD(getShaderiv, PFNglGetShaderiv, "glGetShaderiv");
    LOAD(getShaderInfoLog, PFNglGetShaderInfoLog, "glGetShaderInfoLog");
    LOAD(createProgram, PFNglCreateProgram, "glCreateProgram");
    LOAD(attachShader, PFNglAttachShader, "glAttachShader");
    LOAD(linkProgram, PFNglLinkProgram, "glLinkProgram");
    LOAD(getProgramiv, PFNglGetProgramiv, "glGetProgramiv");
    LOAD(getProgramInfoLog, PFNglGetProgramInfoLog, "glGetProgramInfoLog");
    LOAD(useProgram, PFNglUseProgram, "glUseProgram");
    LOAD(deleteShader, PFNglDeleteShader, "glDeleteShader");
    LOAD(deleteProgram, PFNglDeleteProgram, "glDeleteProgram");
    LOAD(getUniformLocation, PFNglGetUniformLocation, "glGetUniformLocation");
    LOAD(uniform1i, PFNglUniform1i, "glUniform1i");
    LOAD(uniform1f, PFNglUniform1f, "glUniform1f");
    LOAD(uniform3f, PFNglUniform3f, "glUniform3f");
    LOAD(activeTexture, PFNglActiveTexture, "glActiveTexture");
    #undef LOAD
    return true;
}

struct CamoViewer {
    HWND hwnd;
    HDC hdc;
    HGLRC glContext;

    XModelSurf model;
    bool hasModel;
    char *modelPath;
    float modelCenter[3];
    float modelRadius;

    float pitch;
    float yaw;
    float distance;
    bool dragging;
    int lastMouseX;
    int lastMouseY;

    GLuint layerTexture[VIEWER_LAYER_COUNT];
    char *layerPath[VIEWER_LAYER_COUNT];

    bool useShader;
    GLuint program;
    GLint layerSamplerLocation[VIEWER_LAYER_COUNT];
    GLint layerEnabledLocation[VIEWER_LAYER_COUNT];

    bool autoRotate;
    bool needsRedraw;
    bool frameTimerActive;
    UINT frameIntervalMs;
    LONGLONG counterFrequency;
    LARGE_INTEGER lastFrameCounter;

    int clientWidth;
    int clientHeight;
};

static void makeCurrent(CamoViewer *viewer) {
    if (wglGetCurrentContext() == viewer->glContext && wglGetCurrentDC() == viewer->hdc) return;
    wglMakeCurrent(viewer->hdc, viewer->glContext);
}

static void startFrameTimer(CamoViewer *viewer) {
    if (viewer->frameTimerActive) return;
    viewer->frameTimerActive = true;
    timeBeginPeriod(1);
    QueryPerformanceCounter(&viewer->lastFrameCounter);
    SetTimer(viewer->hwnd, VIEWER_TIMER_ID, viewer->frameIntervalMs, NULL);
}

static void stopFrameTimer(CamoViewer *viewer) {
    if (!viewer->frameTimerActive) return;
    viewer->frameTimerActive = false;
    KillTimer(viewer->hwnd, VIEWER_TIMER_ID);
    timeEndPeriod(1);
}

static void requestRedraw(CamoViewer *viewer) {
    viewer->needsRedraw = true;
    startFrameTimer(viewer);
}

static float fitDistance(float radius) {
    return radius / sinf(degToRad(VIEWER_FOV_DEG * 0.5f)) * VIEWER_FIT_MARGIN;
}

static void computeModelBounds(CamoViewer *viewer) {
    float minPos[3] = { FLT_MAX, FLT_MAX, FLT_MAX };
    float maxPos[3] = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
    bool hasVertices = false;

    for (int s = 0; s < viewer->model.surfaceCount; s++) {
        const XSSurface *surface = &viewer->model.surfaces[s];
        for (int v = 0; v < surface->vertexCount; v++) {
            const float *position = surface->vertices[v].position;
            for (int axis = 0; axis < 3; axis++) {
                if (position[axis] < minPos[axis]) minPos[axis] = position[axis];
                if (position[axis] > maxPos[axis]) maxPos[axis] = position[axis];
            }
            hasVertices = true;
        }
    }

    if (!hasVertices) {
        viewer->modelCenter[0] = 0.0f;
        viewer->modelCenter[1] = 0.0f;
        viewer->modelCenter[2] = 0.0f;
        viewer->modelRadius = 1.0f;
        viewer->distance = fitDistance(viewer->modelRadius);
        return;
    }

    float radiusSquared = 0.0f;
    for (int axis = 0; axis < 3; axis++) {
        viewer->modelCenter[axis] = (minPos[axis] + maxPos[axis]) * 0.5f;
        float halfExtent = (maxPos[axis] - minPos[axis]) * 0.5f;
        radiusSquared += halfExtent * halfExtent;
    }

    viewer->modelRadius = sqrtf(radiusSquared);
    if (viewer->modelRadius < 0.001f) viewer->modelRadius = 1.0f;
    viewer->distance = fitDistance(viewer->modelRadius);
}

static GLuint compileShaderResource(GLenum type, int resourceId) {
    void *data = NULL;
    DWORD size = 0;
    if (!resourcesGetData(resourceId, &data, &size)) {
        LOG_ERROR("Camo viewer: missing shader resource %d", resourceId);
        return 0;
    }

    const GLchar *source = (const GLchar *)data;
    GLint length = (GLint)size;

    GLuint shader = gl.createShader(type);
    gl.shaderSource(shader, 1, &source, &length);
    gl.compileShader(shader);

    GLint compiled = 0;
    gl.getShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        char log[1024] = {0};
        gl.getShaderInfoLog(shader, sizeof(log) - 1, NULL, log);
        LOG_ERROR("Camo viewer: shader compile failed: %s", log);
        gl.deleteShader(shader);
        return 0;
    }
    return shader;
}

static void initShaderUniforms(CamoViewer *viewer) {
    for (int layer = 0; layer < VIEWER_LAYER_COUNT; layer++) {
        viewer->layerSamplerLocation[layer] =
            gl.getUniformLocation(viewer->program, LAYER_SAMPLER_UNIFORM[layer]);
        viewer->layerEnabledLocation[layer] =
            gl.getUniformLocation(viewer->program, LAYER_ENABLED_UNIFORM[layer]);
    }

    gl.useProgram(viewer->program);
    for (int layer = 0; layer < VIEWER_LAYER_COUNT; layer++) {
        gl.uniform1i(viewer->layerSamplerLocation[layer], layer);
    }
    gl.uniform3f(gl.getUniformLocation(viewer->program, "lightDir"), 0.4f, 0.6f, 1.0f);
    gl.uniform1f(gl.getUniformLocation(viewer->program, "envStrength"), 0.1f);
    gl.uniform1f(gl.getUniformLocation(viewer->program, "brightness"), 1.0f);
    gl.uniform1i(gl.getUniformLocation(viewer->program, "useStudio"), 1);
    gl.useProgram(0);
}

static bool buildShaderProgram(CamoViewer *viewer) {
    GLuint vertexShader = compileShaderResource(GL_VERTEX_SHADER, IDR_SHADER_CAMO_VIEWER_VERT);
    GLuint fragmentShader = compileShaderResource(GL_FRAGMENT_SHADER, IDR_SHADER_CAMO_VIEWER_FRAG);
    if (!vertexShader || !fragmentShader) {
        if (vertexShader) gl.deleteShader(vertexShader);
        if (fragmentShader) gl.deleteShader(fragmentShader);
        return false;
    }

    viewer->program = gl.createProgram();
    gl.attachShader(viewer->program, vertexShader);
    gl.attachShader(viewer->program, fragmentShader);
    gl.linkProgram(viewer->program);

    GLint linked = 0;
    gl.getProgramiv(viewer->program, GL_LINK_STATUS, &linked);
    gl.deleteShader(vertexShader);
    gl.deleteShader(fragmentShader);
    if (!linked) {
        char log[1024] = {0};
        gl.getProgramInfoLog(viewer->program, sizeof(log) - 1, NULL, log);
        LOG_ERROR("Camo viewer: shader link failed: %s", log);
        gl.deleteProgram(viewer->program);
        viewer->program = 0;
        return false;
    }

    initShaderUniforms(viewer);
    return true;
}

static void initGLState(void) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    glClearColor(0.13f, 0.13f, 0.15f, 1.0f);
}

static GLuint uploadTexture(ViewerLayer layer, const char *iwiPath) {
    IwiImage image;
    char error[128] = {0};
    if (!iwiLoad(iwiPath, &image, error, sizeof(error))) {
        LOG_WARN("Camo viewer: failed to load layer %d '%s': %s", (int)layer, iwiPath, error);
        return 0;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLint wrap = (layer == VIEWER_LAYER_ENV) ? GL_CLAMP_TO_EDGE : GL_REPEAT;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, image.pixels);

    iwiFree(&image);
    return texture;
}

static void releaseLayer(CamoViewer *viewer, ViewerLayer layer) {
    if (viewer->layerTexture[layer]) {
        glDeleteTextures(1, &viewer->layerTexture[layer]);
        viewer->layerTexture[layer] = 0;
    }
    free(viewer->layerPath[layer]);
    viewer->layerPath[layer] = NULL;
}

static void releaseAllLayers(CamoViewer *viewer) {
    for (int layer = 0; layer < VIEWER_LAYER_COUNT; layer++) {
        releaseLayer(viewer, (ViewerLayer)layer);
    }
}

static void syncLayer(CamoViewer *viewer, ViewerLayer layer, const char *path) {
    if (viewer->layerPath[layer] && path && strcmp(viewer->layerPath[layer], path) == 0) return;

    releaseLayer(viewer, layer);
    if (!path) return;

    viewer->layerTexture[layer] = uploadTexture(layer, path);
    viewer->layerPath[layer] = _strdup(path);
}

static void setProjection(CamoViewer *viewer, int width, int height) {
    if (width <= 0) width = 1;
    if (height <= 0) height = 1;
    viewer->clientWidth = width;
    viewer->clientHeight = height;

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float radius = viewer->hasModel ? viewer->modelRadius : 1.0f;
    float nearZ = radius * 0.05f;
    if (nearZ < 0.01f) nearZ = 0.01f;
    float farZ = viewer->distance + radius * 4.0f;
    float top = nearZ * tanf(degToRad(VIEWER_FOV_DEG * 0.5f));
    float right = top * ((float)width / (float)height);
    glFrustum(-right, right, -top, top, nearZ, farZ);
    glMatrixMode(GL_MODELVIEW);
}

static void drawModel(CamoViewer *viewer, bool withUV) {
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    if (withUV) glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    for (int s = 0; s < viewer->model.surfaceCount; s++) {
        const XSSurface *surface = &viewer->model.surfaces[s];
        if (surface->vertexCount == 0 || surface->indexCount == 0) continue;
        const char *base = (const char *)surface->vertices;
        glVertexPointer(3, GL_FLOAT, sizeof(XSVertex), base + offsetof(XSVertex, position));
        glNormalPointer(GL_FLOAT, sizeof(XSVertex), base + offsetof(XSVertex, normal));
        if (withUV)
            glTexCoordPointer(2, GL_FLOAT, sizeof(XSVertex), base + offsetof(XSVertex, uv));
        glDrawElements(GL_TRIANGLES, surface->indexCount, GL_UNSIGNED_SHORT, surface->indices);
    }

    if (withUV) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
}

static void drawShaded(CamoViewer *viewer) {
    gl.useProgram(viewer->program);
    for (int layer = 0; layer < VIEWER_LAYER_COUNT; layer++) {
        gl.activeTexture(GL_TEXTURE0 + layer);
        glBindTexture(GL_TEXTURE_2D, viewer->layerTexture[layer]);
        gl.uniform1i(viewer->layerEnabledLocation[layer], viewer->layerTexture[layer] ? 1 : 0);
    }
    gl.activeTexture(GL_TEXTURE0);
    drawModel(viewer, true);
    gl.useProgram(0);
}

static void drawFixedFunction(CamoViewer *viewer) {
    static const GLfloat lightPosition[] = { 0.4f, 0.6f, 1.0f, 0.0f };
    static const GLfloat materialDiffuse[] = { 0.75f, 0.76f, 0.8f, 1.0f };

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, materialDiffuse);

    GLuint colorTexture = viewer->layerTexture[VIEWER_LAYER_COLOR];
    if (colorTexture) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, colorTexture);
        glColor3f(1.0f, 1.0f, 1.0f);
    }

    drawModel(viewer, colorTexture != 0);

    if (colorTexture) glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
}

static void render(CamoViewer *viewer) {
    makeCurrent(viewer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    if (!viewer->hasModel) {
        SwapBuffers(viewer->hdc);
        return;
    }

    glTranslatef(0.0f, 0.0f, -viewer->distance);
    glRotatef(viewer->pitch, 1.0f, 0.0f, 0.0f);
    glRotatef(viewer->yaw, 0.0f, 1.0f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    glTranslatef(-viewer->modelCenter[0], -viewer->modelCenter[1], -viewer->modelCenter[2]);

    if (viewer->useShader) {
        drawShaded(viewer);
    } else {
        drawFixedFunction(viewer);
    }

    SwapBuffers(viewer->hdc);
}

static float consumeFrameSeconds(CamoViewer *viewer) {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    double seconds = (double)(now.QuadPart - viewer->lastFrameCounter.QuadPart) /
                     (double)viewer->counterFrequency;
    viewer->lastFrameCounter = now;
    if (seconds > VIEWER_MAX_FRAME_SECONDS) seconds = VIEWER_MAX_FRAME_SECONDS;
    return (float)seconds;
}

static void onFrameTick(CamoViewer *viewer) {
    if (!IsWindowVisible(viewer->hwnd)) {
        stopFrameTimer(viewer);
        return;
    }

    if (viewer->autoRotate && viewer->hasModel) {
        float degrees = VIEWER_AUTOROTATE_DEG_PER_SEC * consumeFrameSeconds(viewer);
        viewer->yaw = fmodf(viewer->yaw + degrees, 360.0f);
        viewer->needsRedraw = true;
    }

    if (!viewer->needsRedraw) {
        stopFrameTimer(viewer);
        return;
    }

    viewer->needsRedraw = false;
    render(viewer);
}

static void orbit(CamoViewer *viewer, int deltaX, int deltaY) {
    viewer->yaw = fmodf(viewer->yaw + deltaX * VIEWER_DRAG_DEG_PER_PIXEL, 360.0f);
    viewer->pitch += deltaY * VIEWER_DRAG_DEG_PER_PIXEL;
    if (viewer->pitch > VIEWER_PITCH_LIMIT_DEG) viewer->pitch = VIEWER_PITCH_LIMIT_DEG;
    if (viewer->pitch < -VIEWER_PITCH_LIMIT_DEG) viewer->pitch = -VIEWER_PITCH_LIMIT_DEG;
}

static void zoom(CamoViewer *viewer, float notches) {
    float minDistance = viewer->modelRadius * VIEWER_MIN_DISTANCE_FACTOR;
    float maxDistance = fitDistance(viewer->modelRadius) * VIEWER_MAX_DISTANCE_FACTOR;
    viewer->distance *= powf(VIEWER_ZOOM_PER_NOTCH, notches);
    if (viewer->distance < minDistance) viewer->distance = minDistance;
    if (viewer->distance > maxDistance) viewer->distance = maxDistance;
    setProjection(viewer, viewer->clientWidth, viewer->clientHeight);
}

static LRESULT CALLBACK viewerSubProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                       UINT_PTR idSubclass, DWORD_PTR refData) {
    CamoViewer *viewer = (CamoViewer *)refData;
    switch (msg) {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            viewer->needsRedraw = false;
            render(viewer);
            EndPaint(hwnd, &ps);
        }
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_SIZE:
        makeCurrent(viewer);
        setProjection(viewer, LOWORD(lParam), HIWORD(lParam));
        requestRedraw(viewer);
        return 0;
    case WM_SHOWWINDOW:
        if (wParam && viewer->autoRotate) startFrameTimer(viewer);
        break;
    case WM_TIMER:
        if (wParam == VIEWER_TIMER_ID) {
            onFrameTick(viewer);
            return 0;
        }
        break;
    case WM_LBUTTONDOWN:
        viewer->dragging = true;
        viewer->lastMouseX = GET_X_LPARAM(lParam);
        viewer->lastMouseY = GET_Y_LPARAM(lParam);
        SetCapture(hwnd);
        return 0;
    case WM_LBUTTONUP:
        viewer->dragging = false;
        ReleaseCapture();
        return 0;
    case WM_CAPTURECHANGED:
        viewer->dragging = false;
        return 0;
    case WM_MOUSEMOVE:
        if (viewer->dragging) {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            orbit(viewer, x - viewer->lastMouseX, y - viewer->lastMouseY);
            viewer->lastMouseX = x;
            viewer->lastMouseY = y;
            requestRedraw(viewer);
        }
        return 0;
    case WM_MOUSEWHEEL:
        if (viewer->hasModel) {
            makeCurrent(viewer);
            zoom(viewer, (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA);
            requestRedraw(viewer);
        }
        return 0;
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, viewerSubProc, idSubclass);
        break;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

static bool createGLContext(CamoViewer *viewer) {
    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixelFormat = ChoosePixelFormat(viewer->hdc, &pfd);
    if (pixelFormat == 0 || !SetPixelFormat(viewer->hdc, pixelFormat, &pfd)) {
        LOG_ERROR("Camo viewer: failed to set pixel format");
        return false;
    }

    viewer->glContext = wglCreateContext(viewer->hdc);
    if (!viewer->glContext) {
        LOG_ERROR("Camo viewer: failed to create GL context");
        return false;
    }

    makeCurrent(viewer);
    return true;
}

static UINT frameIntervalFromDisplay(HDC hdc) {
    int refreshHz = GetDeviceCaps(hdc, VREFRESH);
    if (refreshHz <= 1) refreshHz = VIEWER_FALLBACK_REFRESH_HZ;
    UINT intervalMs = (UINT)(1000 / refreshHz);
    return intervalMs < 1 ? 1 : intervalMs;
}

CamoViewer *camoViewerCreate(void *hostHwnd) {
    HWND hwnd = (HWND)hostHwnd;
    if (!hwnd) return NULL;

    CamoViewer *viewer = (CamoViewer *)calloc(1, sizeof(CamoViewer));
    if (!viewer) return NULL;

    viewer->hwnd = hwnd;
    viewer->hdc = GetDC(hwnd);
    if (!createGLContext(viewer)) {
        if (viewer->glContext) wglDeleteContext(viewer->glContext);
        ReleaseDC(hwnd, viewer->hdc);
        free(viewer);
        return NULL;
    }

    viewer->useShader = loadShaderApi() && buildShaderProgram(viewer);
    if (!viewer->useShader) {
        LOG_WARN("Camo viewer: shaders unavailable, using fixed-function fallback");
    }
    initGLState();

    viewer->pitch = VIEWER_INITIAL_PITCH_DEG;
    viewer->yaw = VIEWER_INITIAL_YAW_DEG;
    viewer->modelRadius = 1.0f;
    viewer->distance = fitDistance(viewer->modelRadius);
    viewer->frameIntervalMs = frameIntervalFromDisplay(viewer->hdc);

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    viewer->counterFrequency = frequency.QuadPart;

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    setProjection(viewer, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);

    SetWindowSubclass(hwnd, viewerSubProc, VIEWER_SUBCLASS_ID, (DWORD_PTR)viewer);
    requestRedraw(viewer);
    return viewer;
}

void camoViewerDestroy(CamoViewer *viewer) {
    if (!viewer) return;

    stopFrameTimer(viewer);
    RemoveWindowSubclass(viewer->hwnd, viewerSubProc, VIEWER_SUBCLASS_ID);

    makeCurrent(viewer);
    releaseAllLayers(viewer);
    if (viewer->program) gl.deleteProgram(viewer->program);
    wglMakeCurrent(NULL, NULL);

    wglDeleteContext(viewer->glContext);
    ReleaseDC(viewer->hwnd, viewer->hdc);
    if (viewer->hasModel) xmodelSurfFree(&viewer->model);
    free(viewer->modelPath);
    free(viewer);
}

static void loadModel(CamoViewer *viewer, const char *path) {
    if (!path) {
        if (viewer->hasModel) {
            xmodelSurfFree(&viewer->model);
            viewer->hasModel = false;
        }
        free(viewer->modelPath);
        viewer->modelPath = NULL;
        return;
    }

    if (viewer->modelPath && strcmp(viewer->modelPath, path) == 0) return;

    char *pathCopy = _strdup(path);
    if (!pathCopy) {
        LOG_ERROR("Camo viewer: out of memory loading model '%s'", path);
        return;
    }

    XModelSurf loaded;
    char error[256] = {0};
    if (!xmodelExportLoad(path, &loaded, error, sizeof(error))) {
        LOG_ERROR("Camo viewer: failed to load model '%s': %s", path, error);
        free(pathCopy);
        return;
    }

    if (viewer->hasModel) xmodelSurfFree(&viewer->model);
    viewer->model = loaded;
    viewer->hasModel = true;

    free(viewer->modelPath);
    viewer->modelPath = pathCopy;

    computeModelBounds(viewer);
    setProjection(viewer, viewer->clientWidth, viewer->clientHeight);
}

void camoViewerSetCamo(CamoViewer *viewer, const CamoViewerRequest *request) {
    if (!viewer) return;

    makeCurrent(viewer);

    loadModel(viewer, request ? request->modelPath : NULL);
    syncLayer(viewer, VIEWER_LAYER_COLOR, request ? request->colorPath : NULL);
    syncLayer(viewer, VIEWER_LAYER_NORMAL, request ? request->normalPath : NULL);
    syncLayer(viewer, VIEWER_LAYER_SPEC, request ? request->specPath : NULL);
    syncLayer(viewer, VIEWER_LAYER_ENV, request ? request->envPath : NULL);

    requestRedraw(viewer);
}

void camoViewerSetAutoRotate(CamoViewer *viewer, bool enabled) {
    if (!viewer || viewer->autoRotate == enabled) return;
    viewer->autoRotate = enabled;
    if (enabled) startFrameTimer(viewer);
}
