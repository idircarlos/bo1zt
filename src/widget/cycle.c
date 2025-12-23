#include "widget/cycle.h"
#include "widget/widget_internal.h"
#include "win/gdiplus.h"
#include "win/resources.h"
#include "resource_ids.h"
#include <windows.h>
#include <GL/gl.h>
#include <stdlib.h>

#define CYCLE_BASE_ALPHA 0.3f
#define CYCLE_ACTIVE_ALPHA 1.0f

typedef struct {
    GLuint textures[WIDGET_CYCLE_POWERUP_COUNT];
    float alphas[WIDGET_CYCLE_POWERUP_COUNT];
    int texWidth;
    int texHeight;
    bool initialized;
} CycleData;

static GLuint loadTextureFromResource(int resourceId) {
    IStream* stream = resourcesCreateStream(resourceId);
    if (!stream) return 0;

    GdipBitmap bitmap = gdipLoadFromStream(stream);
    resourcesReleaseStream(stream);
    if (!bitmap) return 0;

    GdipPixelData pixels;
    if (!gdipGetPixels(bitmap, &pixels)) {
        gdipFreeBitmap(bitmap);
        return 0;
    }
    gdipFreeBitmap(bitmap);

    GLuint texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pixels.width, pixels.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.pixels);

    free(pixels.pixels);
    return texId;
}

static void cycleRender(Widget* widget) {
    if (!widget || !widget->displayData) return;
    CycleData* data = (CycleData*)widget->displayData;

    HDC hdc_win = GetDC(widget->win.hwnd);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    // Load textures on first render (OpenGL context is ready)
    if (!data->initialized) {
        static const int powerupResources[WIDGET_CYCLE_POWERUP_COUNT] = {
            IDR_PNG_AMMO,       // POWERUP_MAX_AMMO
            IDR_PNG_INSTA,      // POWERUP_INSTA_KILL
            IDR_PNG_NUKE,       // POWERUP_NUKE
            IDR_PNG_DOUBLE,     // POWERUP_DOUBLE_POINTS
            IDR_PNG_CARPENTER,  // POWERUP_CARPENTER
            IDR_PNG_SALE,       // POWERUP_FIRE_SALE
            IDR_PNG_MINI        // POWERUP_MINI
        };
        for (int i = 0; i < WIDGET_CYCLE_POWERUP_COUNT; i++) {
            data->textures[i] = loadTextureFromResource(powerupResources[i]);
            data->alphas[i] = CYCLE_BASE_ALPHA;
        }
        data->texWidth = WIDGET_CYCLE_ICON_SIZE;
        data->texHeight = WIDGET_CYCLE_ICON_SIZE;
        data->initialized = true;
    }

    glEnable(GL_TEXTURE_2D);

    float totalWidth = (float)widget->render.w;
    float totalHeight = (float)widget->render.h;
    
    // Calculate scale based on widget size vs initial size (266x44)
    float initialWidth = 266.0f;
    float initialHeight = 44.0f;
    float scale = totalWidth / initialWidth;
    float scaleH = totalHeight / initialHeight;
    if (scaleH < scale) scale = scaleH;
    
    float iconSize = (float)WIDGET_CYCLE_ICON_SIZE * scale;
    float padding = (float)WIDGET_CYCLE_ICON_PADDING * scale;

    float allIconsWidth = WIDGET_CYCLE_POWERUP_COUNT * iconSize + (WIDGET_CYCLE_POWERUP_COUNT - 1) * padding;
    float startX = (totalWidth - allIconsWidth) / 2.0f;
    float startY = (totalHeight - iconSize) / 2.0f;

    for (int i = 0; i < WIDGET_CYCLE_POWERUP_COUNT; i++) {
        if (!data->textures[i]) continue;

        float x = startX + i * (iconSize + padding);
        float y = startY;

        float x1 = (x / totalWidth) * 2.0f - 1.0f;
        float y1 = (y / totalHeight) * 2.0f - 1.0f;
        float x2 = ((x + iconSize) / totalWidth) * 2.0f - 1.0f;
        float y2 = ((y + iconSize) / totalHeight) * 2.0f - 1.0f;

        glBindTexture(GL_TEXTURE_2D, data->textures[i]);
        float r = widget->render.textColor.r / 255.0f;
        float g = widget->render.textColor.g / 255.0f;
        float b = widget->render.textColor.b / 255.0f;
        glColor4f(r, g, b, data->alphas[i]);

        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(x1, y1);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(x2, y1);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(x2, y2);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(x1, y2);
        glEnd();
    }

    glDisable(GL_TEXTURE_2D);
    glFinish();

    widgetUpdateLayeredWindow(widget, hdc_win);
    ReleaseDC(widget->win.hwnd, hdc_win);
}

static void cycleDestroy(Widget* widget) {
    if (!widget) return;
    CycleData* data = (CycleData*)widget->displayData;
    if (data) {
        for (int i = 0; i < WIDGET_CYCLE_POWERUP_COUNT; i++) {
            if (data->textures[i]) {
                glDeleteTextures(1, &data->textures[i]);
            }
        }
        free(data);
        widget->displayData = NULL;
    }
}

static WidgetVTable cycleVTable = {
    .render = cycleRender,
    .destroy = cycleDestroy
};

Widget* cycleWidgetCreate(void) {
    CycleData* data = (CycleData*)calloc(1, sizeof(CycleData));
    if (!data) return NULL;

    for (int i = 0; i < WIDGET_CYCLE_POWERUP_COUNT; i++) {
        data->alphas[i] = CYCLE_BASE_ALPHA;
    }

    return widgetCreate("PowerupCycle", &cycleVTable, data, WIDGET_CYCLE_RECT, 0);
}

void cycleWidgetActivate(Widget* cycle, Powerup powerup) {
    if (!cycle || !cycle->displayData) return;
    if (powerup < 0 || powerup >= WIDGET_CYCLE_POWERUP_COUNT) return;

    CycleData* data = (CycleData*)cycle->displayData;
    data->alphas[powerup] = CYCLE_ACTIVE_ALPHA;
}

void cycleWidgetReset(Widget* cycle) {
    if (!cycle || !cycle->displayData) return;

    CycleData* data = (CycleData*)cycle->displayData;
    for (int i = 0; i < WIDGET_CYCLE_POWERUP_COUNT; i++) {
        data->alphas[i] = CYCLE_BASE_ALPHA;
    }
}

bool cycleWidgetIsActive(const Widget* cycle, Powerup powerup) {
    if (!cycle || !cycle->displayData) return false;
    if (powerup < 0 || powerup >= WIDGET_CYCLE_POWERUP_COUNT) return false;

    const CycleData* data = (const CycleData*)cycle->displayData;
    return data->alphas[powerup] >= CYCLE_ACTIVE_ALPHA;
}
