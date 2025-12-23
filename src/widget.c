#include "widget/widget_internal.h"
#include "win/thread.h"
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <windows.h>

#define WIDGET_TARGET_FPS 185
#define WIDGET_RESIZE_EDGE_SIZE 12
#define WIDGET_MIN_WIDTH 150
#define WIDGET_MIN_HEIGHT 50
#define WIDGET_ASCII_CHARS 96
#define WIDGET_ASCII_OFFSET 32
#define WIDGET_ALT_BG_ALPHA 0x20

enum ResizeEdge {
    EDGE_TOP_LEFT = 0,
    EDGE_TOP_RIGHT = 1,
    EDGE_BOTTOM_LEFT = 2,
    EDGE_BOTTOM_RIGHT = 3,
    EDGE_NONE = -1,
};

// Helper: Initialize BITMAPINFO structure for DIB operations
static void initBitmapInfo(BITMAPINFO* bmi, int width, int height) {
    memset(bmi, 0, sizeof(BITMAPINFO));
    bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi->bmiHeader.biWidth = width;
    bmi->bmiHeader.biHeight = height;
    bmi->bmiHeader.biPlanes = 1;
    bmi->bmiHeader.biBitCount = 32;
    bmi->bmiHeader.biCompression = BI_RGB;
}

// Helper: Set window transparency state based on ALT key
static void updateWindowTransparency(Widget* wgt, bool isAltHeld) {
    LONG_PTR cur = GetWindowLongPtr(wgt->win.hwnd, GWL_EXSTYLE);
    bool isAltSaved = flagsContains(wgt->win.flags, WINDOW_ALT_SAVED);
    
    if (isAltHeld && !isAltSaved) {
        wgt->win.savedExstyle = cur;
        flagsAdd(&wgt->win.flags, WINDOW_ALT_SAVED);
        if (cur & WS_EX_TRANSPARENT) {
            SetWindowLongPtr(wgt->win.hwnd, GWL_EXSTYLE, cur & ~WS_EX_TRANSPARENT);
            SetWindowPos(wgt->win.hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }
    } else if (!isAltHeld && isAltSaved) {
        SetWindowLongPtr(wgt->win.hwnd, GWL_EXSTYLE, wgt->win.savedExstyle);
        SetWindowPos(wgt->win.hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        wgt->win.savedExstyle = 0;
        flagsRemove(&wgt->win.flags, WINDOW_ALT_SAVED);
    }
}

// Helper: Calculate new size and scale for resize operation
static void calculateResizeScale(Widget* wgt, int dx, int dy, int* out_w, int* out_h, int* out_font_size) {
    (void)dy;
    float scale = (float)(wgt->win.initialSize.cx + dx) / (float)wgt->win.initialSize.cx;
    
    // Constrain scale based on minimum dimensions
    float min_scale_w = (float)WIDGET_MIN_WIDTH / (float)wgt->win.initialSize.cx;
    float min_scale_h = (float)WIDGET_MIN_HEIGHT / (float)wgt->win.initialSize.cy;
    if (scale < min_scale_w) scale = min_scale_w;
    if (scale < min_scale_h) scale = min_scale_h;
    
    *out_w = (int)(wgt->win.initialSize.cx * scale);
    *out_h = (int)(wgt->win.initialSize.cy * scale);
    *out_font_size = (int)(wgt->render.initialFontSize * scale);
}

static int getResizeEdge(Widget* wgt, int x, int y) {
    if (!wgt) return EDGE_NONE;
    RECT r;
    GetWindowRect(wgt->win.hwnd, &r);
    x -= r.left;
    y -= r.top;

    int w = r.right - r.left;
    int h = r.bottom - r.top;
    bool is_left = x < WIDGET_RESIZE_EDGE_SIZE;
    bool is_right = x >= w - WIDGET_RESIZE_EDGE_SIZE;
    bool is_top = y < WIDGET_RESIZE_EDGE_SIZE;
    bool is_bottom = y >= h - WIDGET_RESIZE_EDGE_SIZE;

    if ((is_left && is_top) || (is_right && is_bottom)) 
        return is_left ? EDGE_TOP_LEFT : EDGE_BOTTOM_RIGHT;
    if ((is_right && is_top) || (is_left && is_bottom)) 
        return is_right ? EDGE_TOP_RIGHT : EDGE_BOTTOM_LEFT;
    return EDGE_NONE;
}

void initGlContext(Widget* wgt) {
    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_BITMAP | PFD_SUPPORT_OPENGL;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    int pf = ChoosePixelFormat(wgt->render.hdcMem, &pfd);
    SetPixelFormat(wgt->render.hdcMem, pf, &pfd);
    wgt->render.glrc = wglCreateContext(wgt->render.hdcMem);
    wglMakeCurrent(wgt->render.hdcMem, wgt->render.glrc);

    glViewport(0, 0, wgt->render.w, wgt->render.h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

/* Helper: recreate the HFONT and the GL display lists for the widget.
 * size == 0 means "keep current size". face == NULL means "keep current face". */
static void recreateFontAndLists(Widget* wgt, const char* face, int size) {
    if (!wgt) return;

    // Delete existing lists/font
    if (wgt->render.listBase) glDeleteLists(wgt->render.listBase, WIDGET_ASCII_CHARS);
    if (wgt->render.font) DeleteObject(wgt->render.font);

    // Update stored face/size if requested
    if (face) {
        free(wgt->render.fontFace);
        wgt->render.fontFace = _strdup(face);
    }
    if (size > 0) wgt->render.fontSize = size;

    wgt->render.font = CreateFontA(wgt->render.fontSize, 0, 0, 0, FW_NORMAL, 
        FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH, wgt->render.fontFace);
    SelectObject(wgt->render.hdcMem, wgt->render.font);
    wgt->render.listBase = glGenLists(WIDGET_ASCII_CHARS);
    wglUseFontBitmapsA(wgt->render.hdcMem, WIDGET_ASCII_OFFSET, WIDGET_ASCII_CHARS, wgt->render.listBase);
}

static void processPendingFont(Widget *wgt) {
    char *face = (char*)InterlockedExchangePointer(&wgt->pending.fontFace, NULL);
    if (face) {
        recreateFontAndLists(wgt, face, 0);
        free(face);
    }
}

static void processPendingFontSize(Widget *wgt) {
    LONG size = InterlockedExchange(&wgt->pending.fontSize, 0);
    if (size > 0) {
        if (!flagsContains(wgt->win.flags, WINDOW_RESIZING))
            wgt->render.initialFontSize = (int)size;
        recreateFontAndLists(wgt, NULL, (int)size);
    }
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Widget* wgt = (Widget*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (!wgt) return DefWindowProc(hwnd, msg, wParam, lParam);

    BOOL altPressed = (GetKeyState(VK_MENU) & 0x8000) != 0;
    switch (msg) {
    case WM_SETCURSOR:
        return TRUE;

    case WM_DESTROY:
        wgt->running = 0;
        return 0;

    case WM_LBUTTONDOWN:
        if (altPressed) {
            POINT pt;
            GetCursorPos(&pt);
            int edge = getResizeEdge(wgt, pt.x, pt.y);
            RECT r;
            GetWindowRect(wgt->win.hwnd, &r);
            
            // Make window clickable if transparent
            LONG_PTR cur_ex = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
            if (wgt->win.savedExstyle == 0) wgt->win.savedExstyle = cur_ex;
            if (cur_ex & WS_EX_TRANSPARENT) {
                SetWindowLongPtr(hwnd, GWL_EXSTYLE, cur_ex & ~WS_EX_TRANSPARENT);
                SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
            }
            
            if (edge >= 0) {
                // Start resizing
                flagsAdd(&wgt->win.flags, WINDOW_RESIZING);
                wgt->win.resizeEdge = edge;
                wgt->win.resizeStart = pt;
                wgt->render.initialFontSize = wgt->render.fontSize;
                wgt->win.initialSize.cx = r.right - r.left;
                wgt->win.initialSize.cy = r.bottom - r.top;
            } else {
                // Start dragging
                wgt->win.dragOffset.x = pt.x - r.left;
                wgt->win.dragOffset.y = pt.y - r.top;
                flagsAdd(&wgt->win.flags, WINDOW_DRAGGING);
            }
            SetCapture(hwnd);
            return 0;
        }
        break;

    case WM_MOUSEMOVE: {
        Flags activeFlags = wgt->win.flags & (WINDOW_DRAGGING | WINDOW_RESIZING);
        bool isAltHeld = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
        
        // Update cursor when not dragging/resizing
        if (!activeFlags) {
            HCURSOR cursor = LoadCursor(NULL, IDC_ARROW);
            if (isAltHeld) {
                POINT pt;
                GetCursorPos(&pt);
                int edge = getResizeEdge(wgt, pt.x, pt.y);
                if (edge == EDGE_TOP_LEFT || edge == EDGE_BOTTOM_RIGHT) 
                    cursor = LoadCursor(NULL, IDC_SIZENWSE);
                else if (edge == EDGE_TOP_RIGHT || edge == EDGE_BOTTOM_LEFT) 
                    cursor = LoadCursor(NULL, IDC_SIZENESW);
                else 
                    cursor = LoadCursor(NULL, IDC_SIZEALL);
            }
            SetCursor(cursor);
        }

        POINT pt;
        GetCursorPos(&pt);
        
        if (flagsContains(wgt->win.flags, WINDOW_DRAGGING)) {
            wgt->pending.pos.x = pt.x - wgt->win.dragOffset.x;
            wgt->pending.pos.y = pt.y - wgt->win.dragOffset.y;
            flagsAdd(&wgt->pending.flags, PENDING_POS);
            return 0;
        }
        
        if (flagsContains(wgt->win.flags, WINDOW_RESIZING)) {
            int dx = pt.x - wgt->win.resizeStart.x;
            int dy = pt.y - wgt->win.resizeStart.y;
            
            if (wgt->win.resizeEdge == EDGE_TOP_LEFT) {
                dx = -dx; dy = -dy;
            } else if (wgt->win.resizeEdge == EDGE_TOP_RIGHT) {
                dy = -dy;
            } else if (wgt->win.resizeEdge == EDGE_BOTTOM_LEFT) {
                dx = -dx;
            }
            
            int new_w, new_h, new_font_size;
            calculateResizeScale(wgt, dx, dy, &new_w, &new_h, &new_font_size);
            
            wgt->pending.size.cx = new_w;
            wgt->pending.size.cy = new_h;
            flagsAdd(&wgt->pending.flags, PENDING_SIZE);
            InterlockedExchange(&wgt->pending.fontSize, (LONG)new_font_size);
            return 0;
        }
        break;
    }

    case WM_LBUTTONUP:
        if (flagsContains(wgt->win.flags, WINDOW_DRAGGING)) {
            flagsRemove(&wgt->win.flags, WINDOW_DRAGGING);
            if (!flagsContains(wgt->win.flags, WINDOW_ALT_SAVED))
                wgt->win.savedExstyle = 0;
            ReleaseCapture();
            return 0;
        }
        if (flagsContains(wgt->win.flags, WINDOW_RESIZING)) {
            RECT r;
            GetWindowRect(wgt->win.hwnd, &r);
            wgt->win.initialSize.cx = r.right - r.left;
            wgt->win.initialSize.cy = r.bottom - r.top;
            wgt->render.initialFontSize = wgt->render.fontSize;
            flagsRemove(&wgt->win.flags, WINDOW_RESIZING);
            ReleaseCapture();
            return 0;
        }
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Helper: Cleanup widget resources (called from thread exit)
static void cleanupWidgetResources(Widget* wgt) {
    // Delete GL resources while context is still current
    if (wgt->render.listBase) glDeleteLists(wgt->render.listBase, WIDGET_ASCII_CHARS);
    if (wgt->render.font) DeleteObject(wgt->render.font);

    // Unbind and delete GL context
    wglMakeCurrent(NULL, NULL);
    if (wgt->render.glrc) wglDeleteContext(wgt->render.glrc);

    if (wgt->render.hbm) DeleteObject(wgt->render.hbm);
    if (wgt->pending.fontFace) free(wgt->pending.fontFace);
    if (wgt->render.fontFace) free(wgt->render.fontFace);
    if (wgt->render.hdcMem) DeleteDC(wgt->render.hdcMem);
    
    DestroyWindow(wgt->win.hwnd);
    if (wgt->displayData) free(wgt->displayData);
    free(wgt);
}

int widgetThreadProc(void *data) {
    Widget* wgt = (Widget*)data;
    if (!wgt || !wgt->vTable || !wgt->vTable->render) return 1;

    MSG msg;
    LARGE_INTEGER freq, last, now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&last);
    double target_frame_ms = 1000.0 / (double)WIDGET_TARGET_FPS;

    initGlContext(wgt);

    while (wgt->running) {        
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (flagsContains(wgt->pending.flags, PENDING_POS)) {
            SetWindowPos(wgt->win.hwnd, NULL, wgt->pending.pos.x, wgt->pending.pos.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
            flagsRemove(&wgt->pending.flags, PENDING_POS);
        }

        if (flagsContains(wgt->pending.flags, PENDING_SIZE)) {
            int old_w = wgt->render.w;
            int old_h = wgt->render.h;
            int new_w = wgt->pending.size.cx;
            int new_h = wgt->pending.size.cy;

            BITMAPINFO bmi;
            initBitmapInfo(&bmi, new_w, new_h);

            HBITMAP newbmp = CreateDIBSection(wgt->render.hdcMem, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);
            if (newbmp) {
                HBITMAP oldbmp = wgt->render.hbm;
                SelectObject(wgt->render.hdcMem, newbmp);
                wgt->render.hbm = newbmp;
                if (oldbmp) {
                    HDC src = CreateCompatibleDC(NULL);
                    if (src) {
                        HBITMAP oldsrc = (HBITMAP)SelectObject(src, oldbmp);
                        StretchBlt(wgt->render.hdcMem, 0, 0, new_w, new_h, src, 0, 0, old_w, old_h, SRCCOPY);
                        if (oldsrc) SelectObject(src, oldsrc);
                        DeleteDC(src);
                    }
                    DeleteObject(oldbmp);
                }
                wgt->render.w = new_w;
                wgt->render.h = new_h;
                glViewport(0, 0, wgt->render.w, wgt->render.h);
                glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(-1,1,-1,1,-1,1);
                glMatrixMode(GL_MODELVIEW); glLoadIdentity();
            }
            flagsRemove(&wgt->pending.flags, PENDING_SIZE);
        }

        // Detect ALT key state each frame and remove/restore WS_EX_TRANSPARENT
        bool isAltHeld = (GetAsyncKeyState(VK_MENU) & 0x8000) ? 1 : 0;
        updateWindowTransparency(wgt, isAltHeld);

        processPendingFont(wgt);
        processPendingFontSize(wgt);
        wgt->vTable->render(wgt);
        QueryPerformanceCounter(&now);
        double elapsed_ms = (double)(now.QuadPart - last.QuadPart) * 1000.0 / (double)freq.QuadPart;
        double remain = target_frame_ms - elapsed_ms;
        if (remain > 1.0) Sleep((DWORD)(remain - 1.0));
        do {
            QueryPerformanceCounter(&now);
            elapsed_ms = (double)(now.QuadPart - last.QuadPart) * 1000.0 / (double)freq.QuadPart;
        } while (elapsed_ms < target_frame_ms);
        last = now;
    }

    // Thread exit: perform cleanup
    cleanupWidgetResources(wgt);
    return 0;
}

void widgetDestroy(Widget* widget) {
    if (!widget) return;
    widget->running = 0; // Signal the widget thread to stop; actual cleanup is performed by the thread.
}

void widgetShow(Widget* widget) {
    if (widget && widget->win.hwnd) {
        ShowWindow(widget->win.hwnd, SW_SHOW);
    }
}

void widgetHide(Widget* widget) {
    if (widget && widget->win.hwnd) {
        ShowWindow(widget->win.hwnd, SW_HIDE);
    }
}

bool widgetIsVisible(const Widget* widget) {
    return widget && widget->win.hwnd && IsWindowVisible(widget->win.hwnd);
}

bool widgetIsTransforming(const Widget* widget) {
    return flagsContains(widget->win.flags, WINDOW_RESIZING | WINDOW_DRAGGING);
}

Rect widgetGetPosition(const Widget* widget) {
    if (!widget) return rectCreate(0, 0, 0, 0);
    
    RECT r;
    GetWindowRect(widget->win.hwnd, &r);
    return rectCreate((uint32_t)r.left, (uint32_t)r.top, (uint32_t)r.right - r.left, (uint32_t)r.bottom - r.top);
}

int widgetGetFontSize(const Widget* widget) {
    return widget->render.fontSize;
}

void widgetSetFontSize(Widget* widget, int fontSize) {
    if (!widget || fontSize <= 0) return;
    InterlockedExchange(&widget->pending.fontSize, (LONG)fontSize);
}

void widgetSetPosition(Widget* widget, Rect rect) {
    if (!widget || !widget->win.hwnd) return;
    
    RECT currentRect;
    GetWindowRect(widget->win.hwnd, &currentRect);
    uint32_t currentW = (uint32_t)(currentRect.right - currentRect.left);
    uint32_t currentH = (uint32_t)(currentRect.bottom - currentRect.top);
    
    UINT flags = SWP_NOZORDER | SWP_NOACTIVATE;
    
    if (rect.w != currentW || rect.h != currentH) {
        SetWindowPos(widget->win.hwnd, HWND_TOPMOST, rect.x, rect.y, rect.w, rect.h, flags);        
        widget->pending.size.cx = rect.w;
        widget->pending.size.cy = rect.h;
        flagsAdd(&widget->pending.flags, PENDING_SIZE);
    } else {
        flags |= SWP_NOSIZE;
        SetWindowPos(widget->win.hwnd, HWND_TOPMOST, rect.x, rect.y, 0, 0, flags);
    }
}

void widgetSetFont(Widget* widget, const char* face) {
    if (!widget || !face) return;
    char* buf = _strdup(face);
    if (!buf) return;
    
    void* prev = InterlockedExchangePointer(&widget->pending.fontFace, buf);
    if (prev) free(prev);
    if (widget->win.hwnd) PostMessageA(widget->win.hwnd, WM_USER + 1, 0, 0);
}

void widgetSetTextColor(Widget* widget, Color color) {
    if (!widget) return;
    // This is a workaround for a known bug that whenever the color is Pure Black (0,0,0) it became trasnparent
    // Since it cannot be differentiated betwenn textColor and bgColor when displaying it.
    // For now, I am setting (1,0,0) which is still ~Pure Black 
    if (color.r == 0 && color.g == 0 && color.b == 0) color.r = 1;
    widget->render.textColor = color;
    if (widget->win.hwnd) {
        PostMessageA(widget->win.hwnd, WM_USER + 1, 0, 0);
    }
}

Widget* widgetCreate(const char* className, WidgetVTable* vTable, void* displayData, Rect rect, int fontSize) {
    HINSTANCE hInst = GetModuleHandle(NULL);
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = className;
    RegisterClassA(&wc);

    Widget* wgt = (Widget*)calloc(1, sizeof(Widget));
    if (!wgt) return NULL;

    wgt->render.w = rect.w;
    wgt->render.h = rect.h;
    wgt->running = 1;
    wgt->vTable = vTable;
    wgt->displayData = displayData;
    wgt->render.fontFace = _strdup("Digital-7 Mono");
    wgt->render.fontSize = fontSize;
    wgt->render.initialFontSize = fontSize;
    wgt->render.textColor = colorCreate(255, 255, 255, 255);
    wgt->win.flags = 0;
    wgt->win.resizeEdge = -1;
    wgt->win.savedExstyle = 0;

    wgt->win.hwnd = CreateWindowExA(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
        className, NULL,
        WS_POPUP,
        rect.x, rect.y, rect.w, rect.h,
        NULL, NULL, hInst, NULL);

    if (!wgt->win.hwnd) {
        free(wgt);
        return NULL;
    }

    ShowWindow(wgt->win.hwnd, 0);

    SetWindowLongPtr(wgt->win.hwnd, GWLP_USERDATA, (LONG_PTR)wgt);

    HDC hdc_screen = GetDC(NULL);
    wgt->render.hdcMem = CreateCompatibleDC(hdc_screen);

    BITMAPINFO bmi;
    initBitmapInfo(&bmi, wgt->render.w, wgt->render.h);
    wgt->render.hbm = CreateDIBSection(hdc_screen, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);
    SelectObject(wgt->render.hdcMem, wgt->render.hbm);

    // Graphics context will be initialized in the widget thread so the OpenGL context is current on that thread.
    ReleaseDC(NULL, hdc_screen);

    Thread *thread = threadCreate(widgetThreadProc, wgt);
    if (!thread) {
        widgetDestroy(wgt);
        return NULL;
    }
    CloseHandle(thread);

    return wgt;
}

void widgetDrawText(Widget* wgt, const char* text) {
    if (!wgt || !text) return;
    // Ensure font & lists exist (create on first use).
    if (!wgt->render.listBase) recreateFontAndLists(wgt, NULL, 0);

    int len = (int)strlen(text);
    SIZE ext = {0,0};
    TEXTMETRIC tm;
    if (len > 0) {
        GetTextExtentPoint32A(wgt->render.hdcMem, text, len, &ext);
        GetTextMetricsA(wgt->render.hdcMem, &tm);
    }

    float text_w = (float)ext.cx;
    float text_h = (float)tm.tmHeight;
    float ascent = (float)tm.tmAscent;

    float px = (wgt->render.w - text_w) * 0.5f;
    float baseline_bottom = (float)wgt->render.h * 0.5f + text_h * 0.5f - ascent;

    float nx = (px / (float)wgt->render.w) * 2.0f - 1.0f;
    float ny = (baseline_bottom / (float)wgt->render.h) * 2.0f - 1.0f;

    glPushAttrib(GL_LIST_BIT | GL_CURRENT_BIT);
    glListBase(wgt->render.listBase - WIDGET_ASCII_OFFSET);
    glColor3ub(wgt->render.textColor.r, wgt->render.textColor.g, wgt->render.textColor.b);
    glRasterPos2f(nx, ny);
    glCallLists(len, GL_UNSIGNED_BYTE, text);
    glPopAttrib();
}

void widgetUpdateLayeredWindow(Widget* wgt, HDC hdc_win) {
    if (!IsWindowVisible(wgt->win.hwnd)) return;
    // Create DIB section for reading GL framebuffer
    BITMAPINFO bmi;
    initBitmapInfo(&bmi, wgt->render.w, wgt->render.h);

    void* bits = NULL;
    HBITMAP tempBmp = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (!tempBmp) return;

    GetDIBits(wgt->render.hdcMem, wgt->render.hbm, 0, wgt->render.h, bits, &bmi, DIB_RGB_COLORS);

    if (bits) {
        int alt_down = (GetAsyncKeyState(VK_MENU) & 0x8000) ? 1 : 0;
        unsigned char bg_alpha = alt_down ? WIDGET_ALT_BG_ALPHA : 0x00;
        unsigned char* px = (unsigned char*)bits;
        int pixels = wgt->render.w * wgt->render.h;
        for (int i = 0; i < pixels; ++i) {
            unsigned char* p = &px[i*4];
            unsigned char b = p[0];
            unsigned char g = p[1];
            unsigned char r = p[2];

            bool isTextPixel = r || g || b;
            unsigned char alpha = isTextPixel ? wgt->render.textColor.a : bg_alpha;

            p[2] = (unsigned char)((((int)r * (int)alpha)) / 255);
            p[1] = (unsigned char)((((int)g * (int)alpha)) / 255);
            p[0] = (unsigned char)((((int)b * (int)alpha)) / 255);
            p[3] = alpha;
        }
    }

    // Update layered window
    POINT ptSrc = {0,0};
    POINT ptDst;
    RECT wr;
    GetWindowRect(wgt->win.hwnd, &wr);
    ptDst.x = wr.left;
    ptDst.y = wr.top;
    SIZE sz = {wgt->render.w, wgt->render.h};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};

    HDC hdc_tmp = CreateCompatibleDC(NULL);
    if (hdc_tmp) {
        HBITMAP oldtmp = (HBITMAP)SelectObject(hdc_tmp, tempBmp);
        UpdateLayeredWindow(wgt->win.hwnd, hdc_win, &ptDst, &sz, hdc_tmp, &ptSrc, 0, &blend, ULW_ALPHA);
        if (oldtmp) SelectObject(hdc_tmp, oldtmp);
        DeleteDC(hdc_tmp);
    }
    DeleteObject(tempBmp);
}
