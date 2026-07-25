#include "uipriv_windows.hpp"

static const wchar_t *GL_AREA_CLASS = L"uiGLAreaClass";

struct uiGLArea {
	uiWindowsControl c;
	HWND hwnd;
	int minWidth;
	int minHeight;
};

static void uiGLAreaDestroy(uiControl *c)
{
	uiGLArea *area = uiGLArea(c);
	uiWindowsEnsureDestroyWindow(area->hwnd);
	uiFreeControl(c);
}

uiWindowsControlAllDefaultsExceptDestroy(uiGLArea)

static void uiGLAreaMinimumSize(uiWindowsControl *c, int *width, int *height)
{
	uiGLArea *area = uiGLArea(c);
	*width = area->minWidth;
	*height = area->minHeight;
}

static void ensureGLAreaClass(void)
{
	static int registered = 0;
	if (registered)
		return;
	WNDCLASSW wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = DefWindowProcW;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
	wc.lpszClassName = GL_AREA_CLASS;
	RegisterClassW(&wc);
	registered = 1;
}

uiGLArea *uiNewGLArea(int minWidth, int minHeight)
{
	uiGLArea *area;

	uiWindowsNewControl(uiGLArea, area);

	area->minWidth = minWidth;
	area->minHeight = minHeight;

	ensureGLAreaClass();
	area->hwnd = uiWindowsEnsureCreateControlHWND(0,
		GL_AREA_CLASS, L"",
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		hInstance, NULL, FALSE);

	return area;
}
