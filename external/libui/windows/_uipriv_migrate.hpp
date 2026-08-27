#ifndef __LIBUI__UIPRIV_MIGRATE_HPP__
#define __LIBUI__UIPRIV_MIGRATE_HPP__

// menu.c
extern void uninitMenus(void);

// draw.c
extern HRESULT initDraw(void);
extern void uninitDraw(void);
extern ID2D1HwndRenderTarget *makeHWNDRenderTarget(HWND hwnd);
extern uiDrawContext *newContext(ID2D1RenderTarget *);
extern void freeContext(uiDrawContext *);

#endif

