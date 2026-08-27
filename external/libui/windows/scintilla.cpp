// uiScintilla - The Scintilla editing component hosted as a libui control
#include "uipriv_windows.hpp"
#include <Scintilla.h>

extern "C" int Scintilla_RegisterClasses(void *hInstance);

struct uiScintilla {
	uiWindowsControl c;
	HWND hwnd;
	void (*onNotify)(uiScintilla *, void *, void *);
	void *onNotifyData;
};

static BOOL onWM_NOTIFY(uiControl *c, HWND hwnd, NMHDR *nmhdr, LRESULT *lResult)
{
	uiScintilla *s = uiScintilla(c);

	(*(s->onNotify))(s, nmhdr, s->onNotifyData);
	*lResult = 0;
	return TRUE;
}

static void uiScintillaDestroy(uiControl *c)
{
	uiScintilla *s = uiScintilla(c);

	uiWindowsUnregisterWM_NOTIFYHandler(s->hwnd);
	uiWindowsEnsureDestroyWindow(s->hwnd);
	uiFreeControl(uiControl(s));
}

uiWindowsControlAllDefaultsExceptDestroy(uiScintilla)

#define scintillaMinWidth 200
#define scintillaMinHeight (14 * 8)

static void uiScintillaMinimumSize(uiWindowsControl *c, int *width, int *height)
{
	uiScintilla *s = uiScintilla(c);
	uiWindowsSizing sizing;
	int x, y;

	x = scintillaMinWidth;
	y = scintillaMinHeight;
	uiWindowsGetSizing(s->hwnd, &sizing);
	uiWindowsSizingDlgUnitsToPixels(&sizing, &x, &y);
	*width = x;
	*height = y;
}

static void defaultOnNotify(uiScintilla *s, void *notification, void *data)
{
	// do nothing
}

void uiScintillaOnNotify(uiScintilla *s, void (*f)(uiScintilla *s, void *notification, void *data), void *data)
{
	s->onNotify = f;
	s->onNotifyData = data;
}

intptr_t uiScintillaSend(uiScintilla *s, unsigned int message, uintptr_t wParam, intptr_t lParam)
{
	return (intptr_t) SendMessageW(s->hwnd, message, (WPARAM) wParam, (LPARAM) lParam);
}

uiScintilla *uiNewScintilla(void)
{
	static bool classesRegistered = false;
	uiScintilla *s;

	if (!classesRegistered) {
		if (Scintilla_RegisterClasses(hInstance) == 0) {
			logLastError(L"error registering the Scintilla window classes");
			return NULL;
		}
		classesRegistered = true;
	}

	uiWindowsNewControl(uiScintilla, s);

	s->hwnd = uiWindowsEnsureCreateControlHWND(0,
		L"Scintilla", L"",
		WS_TABSTOP | WS_HSCROLL | WS_VSCROLL,
		hInstance, NULL,
		FALSE);

	uiWindowsRegisterWM_NOTIFYHandler(s->hwnd, onWM_NOTIFY, uiControl(s));
	uiScintillaOnNotify(s, defaultOnNotify, NULL);

	return s;
}
