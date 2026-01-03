// uiLink - A clickable hyperlink control
#include "uipriv_windows.hpp"
#include <shellapi.h>

// Standard link blue color
#define LINK_COLOR RGB(0, 102, 204)

struct uiLink {
	uiWindowsControl c;
	HWND hwnd;
	char *url;
	HFONT hUnderlineFont;
};

// Parent subclass to handle WM_CTLCOLORSTATIC
static LRESULT CALLBACK parentSubProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	uiLink *l = (uiLink *)dwRefData;

	if (uMsg == WM_CTLCOLORSTATIC && (HWND)lParam == l->hwnd) {
		HDC hdc = (HDC)wParam;
		SetTextColor(hdc, LINK_COLOR);
		SetBkMode(hdc, TRANSPARENT);
		return (LRESULT)GetStockObject(NULL_BRUSH);
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

static LRESULT CALLBACK linkSubProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	uiLink *l = (uiLink *)dwRefData;

	switch (uMsg) {
	case WM_LBUTTONUP:
		if (l->url != NULL) {
			ShellExecuteA(NULL, "open", l->url, NULL, NULL, SW_SHOWNORMAL);
		}
		return 0;

	case WM_SETCURSOR:
		SetCursor(LoadCursor(NULL, IDC_HAND));
		return TRUE;

	case WM_NCDESTROY:
		RemoveWindowSubclass(hwnd, linkSubProc, uIdSubclass);
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

static void uiLinkDestroy(uiControl *c)
{
	uiLink *l = uiLink(c);

	// Remove parent subclass
	HWND parent = GetParent(l->hwnd);
	if (parent)
		RemoveWindowSubclass(parent, parentSubProc, (UINT_PTR)l);

	if (l->url != NULL)
		uiprivFree(l->url);
	if (l->hUnderlineFont != NULL)
		DeleteObject(l->hUnderlineFont);

	uiWindowsEnsureDestroyWindow(l->hwnd);
	uiFreeControl(c);
}

uiWindowsControlDefaultHandle(uiLink)
uiWindowsControlDefaultParent(uiLink)
uiWindowsControlDefaultSetParent(uiLink)
uiWindowsControlDefaultToplevel(uiLink)
uiWindowsControlDefaultVisible(uiLink)
uiWindowsControlDefaultShow(uiLink)
uiWindowsControlDefaultHide(uiLink)
uiWindowsControlDefaultEnabled(uiLink)
uiWindowsControlDefaultEnable(uiLink)
uiWindowsControlDefaultDisable(uiLink)
uiWindowsControlDefaultSyncEnableState(uiLink)
// Custom SetParentHWND defined below
uiWindowsControlDefaultMinimumSizeChanged(uiLink)
uiWindowsControlDefaultLayoutRect(uiLink)
uiWindowsControlDefaultAssignControlIDZOrder(uiLink)
uiWindowsControlDefaultChildVisibilityChanged(uiLink)

static void uiLinkMinimumSize(uiWindowsControl *c, int *width, int *height)
{
	uiLink *l = uiLink(c);
	uiWindowsSizing sizing;
	int y;

	*width = uiWindowsWindowTextWidth(l->hwnd);
	y = uiWindowsWindowTextHeight(l->hwnd);
	uiWindowsGetSizing(l->hwnd, &sizing);
	uiWindowsSizingDlgUnitsToPixels(&sizing, NULL, &y);
	*height = y;
}

char *uiLinkText(uiLink *l)
{
	return uiWindowsWindowText(l->hwnd);
}

void uiLinkSetText(uiLink *l, const char *text)
{
	uiWindowsSetWindowText(l->hwnd, text);
	uiWindowsControlMinimumSizeChanged(uiWindowsControl(l));
}

const char *uiLinkURL(uiLink *l)
{
	return l->url;
}

void uiLinkSetURL(uiLink *l, const char *url)
{
	if (l->url != NULL)
		uiprivFree(l->url);
	
	if (url != NULL) {
		size_t len = strlen(url) + 1;
		l->url = (char *)uiprivAlloc(len, "char[]");
		strcpy(l->url, url);
	} else {
		l->url = NULL;
	}
}

// Custom SetParentHWND to install parent subclass for color handling
static void uiLinkSetParentHWND(uiWindowsControl *c, HWND parent)
{
	uiLink *l = uiLink(c);
	
	// Remove subclass from old parent
	HWND oldParent = GetParent(l->hwnd);
	if (oldParent)
		RemoveWindowSubclass(oldParent, parentSubProc, (UINT_PTR)l);
	
	// Set new parent
	uiWindowsEnsureSetParentHWND(l->hwnd, parent);
	
	// Install subclass on new parent for WM_CTLCOLORSTATIC
	if (parent)
		SetWindowSubclass(parent, parentSubProc, (UINT_PTR)l, (DWORD_PTR)l);
}

uiLink *uiNewLink(const char *text, const char *url)
{
	uiLink *l;
	WCHAR *wtext;

	uiWindowsNewControl(uiLink, l);

	wtext = toUTF16(text);
	l->hwnd = uiWindowsEnsureCreateControlHWND(0,
		L"static", wtext,
		SS_NOTIFY | SS_LEFTNOWORDWRAP,
		hInstance, NULL,
		TRUE);
	uiprivFree(wtext);

	// Set underline font
	HFONT hFont = (HFONT)SendMessage(l->hwnd, WM_GETFONT, 0, 0);
	LOGFONTW lf;
	GetObjectW(hFont, sizeof(lf), &lf);
	lf.lfUnderline = TRUE;
	l->hUnderlineFont = CreateFontIndirectW(&lf);
	SendMessage(l->hwnd, WM_SETFONT, (WPARAM)l->hUnderlineFont, TRUE);

	// Store URL
	l->url = NULL;
	uiLinkSetURL(l, url);

	// Override SetParentHWND to handle color
	uiWindowsControl(l)->SetParentHWND = uiLinkSetParentHWND;

	SetWindowSubclass(l->hwnd, linkSubProc, 0, (DWORD_PTR)l);

	return l;
}
