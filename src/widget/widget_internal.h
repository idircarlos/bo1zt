#ifndef WIDGET_INTERNAL_H_
#define WIDGET_INTERNAL_H_

#include "widget.h"
#include <GL/gl.h>

struct WidgetVTable {
	void (*render)(Widget* widget);
	void (*destroy)(Widget* widget);
};

typedef enum {
	WINDOW_DRAGGING = 0x1,
	WINDOW_RESIZING = 0x2,
	WINDOW_CTRL_SAVED = 0x4,
} PendingWindowFlags;

typedef struct WindowState {
    HWND hwnd;
    LONG_PTR savedExstyle;
    POINT dragOffset;
	Flags flags;
	POINT resizeStart;
	SIZE initialSize;
	int resizeEdge;
} WindowState;

typedef struct RenderState {
    HDC hdcMem;
    HBITMAP hbm;
    HGLRC glrc;
    HFONT font;
    GLuint listBase;
    char* fontFace;
    int fontSize;
	int initialFontSize;
	int w;
	int h;
    Color textColor;
} RenderState;

typedef enum {
	PENDING_POS = 0x1,
	PENDING_SIZE = 0x2,
} PendingStateFlags;

typedef struct PendingState {
	POINT pos;
	SIZE size;
	Flags flags;
	void* fontFace; // heap-allocated char* set by other threads
	LONG fontSize;  // atomic LONG; 0 == no pending size
} PendingState;

struct Widget {
	WindowState win;
	RenderState render;
	PendingState pending;
	WidgetVTable* vTable;
    int running;  
	void* displayData;
};

Widget* widgetCreate(const char* className, const char* windowTitle, WidgetVTable* vTable, void* displayData, int x, int y, int width, int height, int fontSize);
void widgetDrawText(Widget* wgt, const char* text);
void widgetUpdateLayeredWindow(Widget* wgt, HDC hdc_win);

#endif // WIDGET_INTERNAL_H_