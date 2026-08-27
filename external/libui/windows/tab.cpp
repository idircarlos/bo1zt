// 16 may 2015
#include "uipriv_windows.hpp"

// You don't add controls directly to a tab control on Windows; instead you make them siblings and swap between them on a TCN_SELCHANGING/TCN_SELCHANGE notification pair.
// In addition, you use dialogs because they can be textured properly; other controls cannot. (Things will look wrong if the tab background in the current theme is fancy if you just use the tab background by itself; see http://stackoverflow.com/questions/30087540/why-are-my-programss-tab-controls-rendering-their-background-in-a-blocky-way-b.)

struct uiTab {
	uiWindowsControl c;
	HWND hwnd;			// of the outer container
	HWND tabHWND;		// of the tab control itself
	std::vector<struct tabPage *> *pages;
	HWND parent;
	void (*onSelected)(uiTab *, void *);
	void *onSelectedData;
	void (*onClosing)(uiTab *, int, void *);
	void *onClosingData;
	int hotCloseButton;
	int pressedCloseButton;
	BOOL trackingMouse;
};

#define tabCloseButtonSize 14
#define tabCloseButtonMargin 4
#define tabCloseButtonRadius 4.0
#define tabCloseButtonSamples 4
#define tabCloseButtonSelectedRise 2
#define tabCloseButtonRise -1
#define tabCloseButtonHotColor RGB(0xD0, 0xD0, 0xD0)
#define tabCloseButtonPressedColor RGB(0xA8, 0xA8, 0xA8)
#define tabCloseCrossInset 4.0
#define tabCloseCrossThickness 0.5
#define tabCloseTextGap 6
#define tabPaddingVertical 3
#define tabPaddingHorizontal 6
#define tabBodyBorder 1

// utility functions

static LRESULT curpage(uiTab *t)
{
	return SendMessageW(t->tabHWND, TCM_GETCURSEL, 0, 0);
}

static struct tabPage *tabPage(uiTab *t, int i)
{
	return (*(t->pages))[i];
}

// TCM_ADJUSTRECT insets the page by the same padding on every side; only the strip height is wanted, so the
// horizontal inset it reports is subtracted back out of the top and the page is left flush against the body frame
static void tabPageRect(uiTab *t, RECT *r)
{
	RECT display;
	int padding;

	// this rect needs to be in parent window coordinates, but TCM_ADJUSTRECT wants a window rect, which is screen coordinates
	// because we have each page as a sibling of the tab, use the tab's own rect as the input rect
	uiWindowsEnsureGetWindowRect(t->tabHWND, &display);
	SendMessageW(t->tabHWND, TCM_ADJUSTRECT, (WPARAM) FALSE, (LPARAM) (&display));
	// and get it in terms of the container instead of the screen
	mapWindowRect(NULL, t->hwnd, &display);

	uiWindowsEnsureGetClientRect(t->hwnd, r);
	padding = display.left - r->left;
	r->left += tabBodyBorder;
	r->top = display.top - padding + tabBodyBorder;
	r->right -= tabBodyBorder;
	r->bottom -= tabBodyBorder;
}

static void tabRelayout(uiTab *t)
{
	struct tabPage *page;
	RECT r;
	LONG_PTR controlID;
	HWND insertAfter;

	// first move the tab control itself
	uiWindowsEnsureGetClientRect(t->hwnd, &r);
	uiWindowsEnsureMoveWindowDuringResize(t->tabHWND, r.left, r.top, r.right - r.left, r.bottom - r.top);

	// then the current page
	if (t->pages->size() == 0)
		return;
	page = tabPage(t, curpage(t));
	tabPageRect(t, &r);
	controlID = 100;
	insertAfter = NULL;
	uiWindowsEnsureMoveWindowDuringResize(page->hwnd, r.left, r.top, r.right - r.left, r.bottom - r.top);
	uiWindowsEnsureAssignControlIDZOrder(page->hwnd, &controlID, &insertAfter);
}

static void showHidePage(uiTab *t, LRESULT which, int hide)
{
	struct tabPage *page;

	if (which == (LRESULT) (-1))
		return;
	page = tabPage(t, which);
	if (hide)
		ShowWindow(page->hwnd, SW_HIDE);
	else {
		ShowWindow(page->hwnd, SW_SHOW);
		// we only resize the current page, so we have to resize it; before we can do that, we need to make sure we are of the right size
		uiWindowsControlMinimumSizeChanged(uiWindowsControl(t));
		(*t->onSelected)(t, t->onSelectedData);
	}
}

// close buttons

static bool tabHasCloseButtons(uiTab *t)
{
	return t->onClosing != NULL;
}

static int tabSpaceWidth(uiTab *t)
{
	HDC dc;
	HFONT font, previousFont;
	SIZE size;

	dc = GetDC(t->tabHWND);
	font = (HFONT) SendMessageW(t->tabHWND, WM_GETFONT, 0, 0);
	previousFont = (HFONT) SelectObject(dc, font);
	if (GetTextExtentPoint32W(dc, L" ", 1, &size) == 0)
		size.cx = 0;
	SelectObject(dc, previousFont);
	ReleaseDC(t->tabHWND, dc);
	return size.cx;
}

// the tab control centers the item label, so the room for the close button has to be part of the label itself
static WCHAR *tabItemText(uiTab *t, const char *name)
{
	WCHAR *text, *padded;
	size_t length;
	int spaceWidth, spaces;

	text = toUTF16(name);
	if (!tabHasCloseButtons(t))
		return text;

	spaceWidth = tabSpaceWidth(t);
	if (spaceWidth <= 0)
		return text;

	spaces = tabCloseButtonMargin + tabCloseButtonSize + tabCloseTextGap - tabPaddingHorizontal;
	spaces = (spaces + spaceWidth - 1) / spaceWidth;

	length = wcslen(text);
	padded = (WCHAR *) uiprivAlloc((length + spaces + 1) * sizeof (WCHAR), "WCHAR[]");
	memcpy(padded, text, length * sizeof (WCHAR));
	wmemset(padded + length, L' ', spaces);
	padded[length + spaces] = L'\0';
	uiprivFree(text);
	return padded;
}

static bool tabItemRect(uiTab *t, int index, RECT *out)
{
	if (index < 0 || index >= (int) (t->pages->size()))
		return false;
	return SendMessageW(t->tabHWND, TCM_GETITEMRECT, (WPARAM) index, (LPARAM) out) != FALSE;
}

static bool tabCloseButtonRect(uiTab *t, int index, RECT *out)
{
	RECT item;
	int top;

	if (!tabHasCloseButtons(t))
		return false;
	if (!tabItemRect(t, index, &item))
		return false;

	top = (item.top + item.bottom - tabCloseButtonSize) / 2 - tabCloseButtonRise;
	// comctl32 grows the selected item and lifts its label with it
	if (index == (int) curpage(t))
		top -= tabCloseButtonSelectedRise;
	out->left = item.right - tabCloseButtonSize - tabCloseButtonMargin;
	out->top = top;
	out->right = out->left + tabCloseButtonSize;
	out->bottom = top + tabCloseButtonSize;
	return true;
}

static int tabCloseButtonAt(uiTab *t, LPARAM pos)
{
	TCHITTESTINFO hit;
	RECT close;
	int index;

	ZeroMemory(&hit, sizeof (TCHITTESTINFO));
	hit.pt.x = GET_X_LPARAM(pos);
	hit.pt.y = GET_Y_LPARAM(pos);
	index = SendMessageW(t->tabHWND, TCM_HITTEST, 0, (LPARAM) (&hit));
	if (index < 0)
		return -1;
	if (!tabCloseButtonRect(t, index, &close))
		return -1;
	return PtInRect(&close, hit.pt) != 0 ? index : -1;
}

static void tabInvalidateItem(uiTab *t, int index)
{
	RECT item, close;

	if (!tabItemRect(t, index, &item))
		return;
	if (tabCloseButtonRect(t, index, &close))
		UnionRect(&item, &item, &close);
	InvalidateRect(t->tabHWND, &item, TRUE);
}

static double tabSegmentDistance(double x, double y, double ax, double ay, double bx, double by)
{
	double dx = bx - ax;
	double dy = by - ay;
	double t = ((x - ax) * dx + (y - ay) * dy) / (dx * dx + dy * dy);

	if (t < 0.0)
		t = 0.0;
	if (t > 1.0)
		t = 1.0;
	x -= ax + t * dx;
	y -= ay + t * dy;
	return sqrt(x * x + y * y);
}

static bool tabPointInCross(double x, double y)
{
	double low = tabCloseCrossInset;
	double high = tabCloseButtonSize - tabCloseCrossInset;

	return tabSegmentDistance(x, y, low, low, high, high) <= tabCloseCrossThickness ||
		tabSegmentDistance(x, y, low, high, high, low) <= tabCloseCrossThickness;
}

static bool tabPointInRoundedRect(double x, double y)
{
	double center = tabCloseButtonSize / 2.0;
	double inner = center - tabCloseButtonRadius;
	double dx = fabs(x - center) - inner;
	double dy = fabs(y - center) - inner;

	if (dx <= 0.0 || dy <= 0.0)
		return true;
	return dx * dx + dy * dy <= tabCloseButtonRadius * tabCloseButtonRadius;
}

static double tabCloseButtonCoverage(int x, int y, bool (*inside)(double, double))
{
	int hits = 0;
	int sx, sy;

	for (sy = 0; sy < tabCloseButtonSamples; sy++)
		for (sx = 0; sx < tabCloseButtonSamples; sx++)
			if ((*inside)(x + (sx + 0.5) / tabCloseButtonSamples,
				y + (sy + 0.5) / tabCloseButtonSamples))
				hits++;
	return (double) hits / (tabCloseButtonSamples * tabCloseButtonSamples);
}

// premultiplied BGRA so that AlphaBlend() composites the cross over whatever the tab painted
static void tabBlendCloseButtonPixel(BYTE *pixel, COLORREF cross, double crossAlpha, COLORREF back, double backAlpha)
{
	double behind = backAlpha * (1.0 - crossAlpha);

	pixel[0] = (BYTE) (GetBValue(cross) * crossAlpha + GetBValue(back) * behind + 0.5);
	pixel[1] = (BYTE) (GetGValue(cross) * crossAlpha + GetGValue(back) * behind + 0.5);
	pixel[2] = (BYTE) (GetRValue(cross) * crossAlpha + GetRValue(back) * behind + 0.5);
	pixel[3] = (BYTE) (255.0 * (crossAlpha + behind) + 0.5);
}

static HBITMAP tabRenderCloseButton(HDC dc, BOOL filled, COLORREF back)
{
	BITMAPINFO info;
	HBITMAP bitmap;
	void *bits = NULL;
	COLORREF cross;
	int x, y;

	ZeroMemory(&info, sizeof (BITMAPINFO));
	info.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
	info.bmiHeader.biWidth = tabCloseButtonSize;
	info.bmiHeader.biHeight = -tabCloseButtonSize;
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = 32;
	info.bmiHeader.biCompression = BI_RGB;

	bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, NULL, 0);
	if (bitmap == NULL) {
		logLastError(L"error creating the uiTab close button bitmap");
		return NULL;
	}

	cross = GetSysColor(COLOR_BTNTEXT);
	for (y = 0; y < tabCloseButtonSize; y++)
		for (x = 0; x < tabCloseButtonSize; x++)
			tabBlendCloseButtonPixel((BYTE *) bits + 4 * (y * tabCloseButtonSize + x),
				cross, tabCloseButtonCoverage(x, y, tabPointInCross),
				back, filled ? tabCloseButtonCoverage(x, y, tabPointInRoundedRect) : 0.0);

	return bitmap;
}

static void tabDrawCloseButton(uiTab *t, HDC dc, int index, const RECT *r)
{
	HDC memDC;
	HBITMAP bitmap;
	HGDIOBJ previousBitmap;
	BLENDFUNCTION blend;
	BOOL filled;

	filled = index == t->pressedCloseButton || index == t->hotCloseButton;
	bitmap = tabRenderCloseButton(dc, filled, index == t->pressedCloseButton ?
		tabCloseButtonPressedColor : tabCloseButtonHotColor);
	if (bitmap == NULL)
		return;

	memDC = CreateCompatibleDC(dc);
	previousBitmap = SelectObject(memDC, bitmap);
	blend.BlendOp = AC_SRC_OVER;
	blend.BlendFlags = 0;
	blend.SourceConstantAlpha = 255;
	blend.AlphaFormat = AC_SRC_ALPHA;
	AlphaBlend(dc, r->left, r->top, tabCloseButtonSize, tabCloseButtonSize,
		memDC, 0, 0, tabCloseButtonSize, tabCloseButtonSize, blend);
	SelectObject(memDC, previousBitmap);
	DeleteDC(memDC);
	DeleteObject(bitmap);
}

// the tab control paints itself first, so this only ever adds the buttons on top of it
// blending a button over a copy of itself darkens it, so only the pixels comctl32 just repainted may be touched
static void tabDrawCloseButtons(uiTab *t, HRGN repainted)
{
	HDC dc;
	RECT close;
	int i;

	dc = GetDC(t->tabHWND);
	if (dc == NULL) {
		logLastError(L"error getting DC to draw the uiTab close buttons");
		return;
	}
	SelectClipRgn(dc, repainted);

	for (i = 0; i < (int) (t->pages->size()); i++)
		if (tabCloseButtonRect(t, i, &close))
			tabDrawCloseButton(t, dc, i, &close);

	SelectClipRgn(dc, NULL);
	ReleaseDC(t->tabHWND, dc);
}

static void tabTrackMouseLeave(uiTab *t)
{
	TRACKMOUSEEVENT track;

	if (t->trackingMouse)
		return;

	ZeroMemory(&track, sizeof (TRACKMOUSEEVENT));
	track.cbSize = sizeof (TRACKMOUSEEVENT);
	track.dwFlags = TME_LEAVE;
	track.hwndTrack = t->tabHWND;
	if (TrackMouseEvent(&track) == 0)
		logLastError(L"error tracking mouse leave in uiTab");
	else
		t->trackingMouse = TRUE;
}

static LRESULT CALLBACK tabSubProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	uiTab *t = (uiTab *) dwRefData;
	LRESULT painted;
	HRGN repainted;
	int index;

	switch (uMsg) {
	case WM_PAINT:
		if (!tabHasCloseButtons(t))
			break;
		repainted = CreateRectRgn(0, 0, 0, 0);
		if (GetUpdateRgn(hwnd, repainted, FALSE) == ERROR) {
			DeleteObject(repainted);
			break;
		}
		painted = DefSubclassProc(hwnd, uMsg, wParam, lParam);
		tabDrawCloseButtons(t, repainted);
		DeleteObject(repainted);
		return painted;
	case WM_MOUSEMOVE:
		if (!tabHasCloseButtons(t))
			break;
		index = tabCloseButtonAt(t, lParam);
		if (index != t->hotCloseButton) {
			tabInvalidateItem(t, t->hotCloseButton);
			t->hotCloseButton = index;
			tabInvalidateItem(t, index);
		}
		if (index >= 0)
			tabTrackMouseLeave(t);
		break;
	case WM_MOUSELEAVE:
		t->trackingMouse = FALSE;
		tabInvalidateItem(t, t->hotCloseButton);
		t->hotCloseButton = -1;
		break;
	case WM_LBUTTONDOWN:
		if (!tabHasCloseButtons(t))
			break;
		index = tabCloseButtonAt(t, lParam);
		if (index >= 0) {
			t->pressedCloseButton = index;
			tabInvalidateItem(t, index);
			return 0;
		}
		break;
	case WM_LBUTTONUP:
		if (t->pressedCloseButton >= 0) {
			index = t->pressedCloseButton;
			t->pressedCloseButton = -1;
			t->hotCloseButton = -1;
			tabInvalidateItem(t, index);
			if (index == tabCloseButtonAt(t, lParam))
				(*(t->onClosing))(t, index, t->onClosingData);
			return 0;
		}
		break;
	case WM_NCDESTROY:
		RemoveWindowSubclass(hwnd, tabSubProc, uIdSubclass);
		break;
	}
	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

// control implementation

static BOOL onWM_NOTIFY(uiControl *c, HWND hwnd, NMHDR *nm, LRESULT *lResult)
{
	uiTab *t = uiTab(c);

	if (nm->code != TCN_SELCHANGING && nm->code != TCN_SELCHANGE)
		return FALSE;
	showHidePage(t, curpage(t), nm->code == TCN_SELCHANGING);
	*lResult = 0;
	if (nm->code == TCN_SELCHANGING)
		*lResult = FALSE;
	return TRUE;
}

static void defaultOnSelected(uiTab *t, void *data)
{
	// do nothing
}

static void uiTabDestroy(uiControl *c)
{
	uiTab *t = uiTab(c);
	uiControl *child;

	uiTabOnSelected(t, defaultOnSelected, NULL);
	for (struct tabPage *&page : *(t->pages)) {
		child = page->child;
		tabPageDestroy(page);
		if (child != NULL) {
			uiControlSetParent(child, NULL);
			uiControlDestroy(child);
		}
	}
	delete t->pages;
	uiWindowsUnregisterWM_NOTIFYHandler(t->tabHWND);
	uiWindowsEnsureDestroyWindow(t->tabHWND);
	uiWindowsEnsureDestroyWindow(t->hwnd);
	uiFreeControl(uiControl(t));
}

uiWindowsControlDefaultHandle(uiTab)
uiWindowsControlDefaultParent(uiTab)
uiWindowsControlDefaultSetParent(uiTab)
uiWindowsControlDefaultToplevel(uiTab)
uiWindowsControlDefaultVisible(uiTab)
uiWindowsControlDefaultShow(uiTab)
uiWindowsControlDefaultHide(uiTab)
uiWindowsControlDefaultEnabled(uiTab)
uiWindowsControlDefaultEnable(uiTab)
uiWindowsControlDefaultDisable(uiTab)

static void uiTabSyncEnableState(uiWindowsControl *c, int enabled)
{
	uiTab *t = uiTab(c);

	if (uiWindowsShouldStopSyncEnableState(uiWindowsControl(t), enabled))
		return;
	EnableWindow(t->tabHWND, enabled);
	for (struct tabPage *&page : *(t->pages))
		if (page->child != NULL)
			uiWindowsControlSyncEnableState(uiWindowsControl(page->child), enabled);
}

uiWindowsControlDefaultSetParentHWND(uiTab)

static void uiTabMinimumSize(uiWindowsControl *c, int *width, int *height)
{
	uiTab *t = uiTab(c);
	int pagewid, pageht;
	struct tabPage *page;
	RECT r;

	// only consider the current page
	pagewid = 0;
	pageht = 0;
	if (t->pages->size() != 0) {
		page = tabPage(t, curpage(t));
		tabPageMinimumSize(page, &pagewid, &pageht);
	}

	r.left = 0;
	r.top = 0;
	r.right = pagewid;
	r.bottom = pageht;
	// this also includes the tabs themselves
	SendMessageW(t->tabHWND, TCM_ADJUSTRECT, (WPARAM) TRUE, (LPARAM) (&r));
	*width = r.right - r.left;
	*height = r.bottom - r.top;
}

static void uiTabMinimumSizeChanged(uiWindowsControl *c)
{
	uiTab *t = uiTab(c);

	if (uiWindowsControlTooSmall(uiWindowsControl(t))) {
		uiWindowsControlContinueMinimumSizeChanged(uiWindowsControl(t));
		return;
	}
	tabRelayout(t);
}

uiWindowsControlDefaultLayoutRect(uiTab)
uiWindowsControlDefaultAssignControlIDZOrder(uiTab)

static void uiTabChildVisibilityChanged(uiWindowsControl *c)
{
	// TODO eliminate the redundancy
	uiWindowsControlMinimumSizeChanged(c);
}

static void tabArrangePages(uiTab *t)
{
	LONG_PTR controlID = 100;
	HWND insertAfter = NULL;

	// TODO is this first or last?
	uiWindowsEnsureAssignControlIDZOrder(t->tabHWND, &controlID, &insertAfter);
	for (struct tabPage *&page : *(t->pages))
		uiWindowsEnsureAssignControlIDZOrder(page->hwnd, &controlID, &insertAfter);
}

void uiTabAppend(uiTab *t, const char *name, uiControl *child)
{
	uiTabInsertAt(t, name, t->pages->size(), child);
}

void uiTabInsertAt(uiTab *t, const char *name, int n, uiControl *child)
{
	struct tabPage *page;
	LRESULT hide, show;
	TCITEMW item;
	WCHAR *wname;

	// see below
	hide = curpage(t);

	if (child != NULL)
		uiControlSetParent(child, uiControl(t));

	page = newTabPage(child);
	uiWindowsEnsureSetParentHWND(page->hwnd, t->hwnd);
	t->pages->insert(t->pages->begin() + n, page);
	tabArrangePages(t);

	ZeroMemory(&item, sizeof (TCITEMW));
	item.mask = TCIF_TEXT;
	wname = tabItemText(t, name);
	item.pszText = wname;
	if (SendMessageW(t->tabHWND, TCM_INSERTITEM, (WPARAM) n, (LPARAM) (&item)) == (LRESULT) -1)
		logLastError(L"error adding tab to uiTab");
	uiprivFree(wname);

	// we need to do this because adding the first tab doesn't send a TCN_SELCHANGE; it just shows the page
	show = curpage(t);
	if (show != hide) {
		showHidePage(t, hide, 1);
		showHidePage(t, show, 0);
	}
}

void uiTabDelete(uiTab *t, int n)
{
	struct tabPage *page;

	// first delete the tab from the tab control
	// if this is the current tab, no tab will be selected, which is good
	if (SendMessageW(t->tabHWND, TCM_DELETEITEM, (WPARAM) n, 0) == FALSE)
		logLastError(L"error deleting uiTab tab");

	// now delete the page itself
	page = tabPage(t, n);
	if (page->child != NULL)
		uiControlSetParent(page->child, NULL);
	tabPageDestroy(page);
	t->pages->erase(t->pages->begin() + n);
}

int uiTabNumPages(uiTab *t)
{
	return t->pages->size();
}

int uiTabMargined(uiTab *t, int n)
{
	return tabPage(t, n)->margined;
}

void uiTabSetMargined(uiTab *t, int n, int margined)
{
	struct tabPage *page;

	page = tabPage(t, n);
	page->margined = margined;
	// even if the page doesn't have a child it might still have a new minimum size with margins; this is the easiest way to verify it
	uiWindowsControlMinimumSizeChanged(uiWindowsControl(t));
}

static void onResize(uiWindowsControl *c)
{
	tabRelayout(uiTab(c));
}

void uiTabOnSelected(uiTab *t, void (*f)(uiTab *, void *), void *data)
{
	t->onSelected = f;
	t->onSelectedData = data;
}

int uiTabSelected(uiTab *t)
{
	return curpage(t);
}

void uiTabSetSelected(uiTab *t, int index)
{
	if (index < 0 || index >= uiTabNumPages(t))
		return;
	showHidePage(t, curpage(t), 1);
	SendMessageW(t->tabHWND, TCM_SETCURSEL, index, 0);
	showHidePage(t, index, 0);
}

void uiTabOnClosing(uiTab *t, void (*f)(uiTab *t, int index, void *data), void *data)
{
	t->onClosing = f;
	t->onClosingData = data;
	t->hotCloseButton = -1;
	t->pressedCloseButton = -1;

	InvalidateRect(t->tabHWND, NULL, TRUE);
	uiWindowsControlMinimumSizeChanged(uiWindowsControl(t));
}

uiTab *uiNewTab(void)
{
	uiTab *t;

	uiWindowsNewControl(uiTab, t);

	t->hwnd = uiWindowsMakeContainer(uiWindowsControl(t), onResize);

	t->tabHWND = uiWindowsEnsureCreateControlHWND(0,
		WC_TABCONTROLW, L"",
		TCS_TOOLTIPS | WS_TABSTOP,
		hInstance, NULL,
		TRUE);
	uiWindowsEnsureSetParentHWND(t->tabHWND, t->hwnd);
	SendMessageW(t->tabHWND, TCM_SETPADDING, 0, MAKELPARAM(tabPaddingHorizontal, tabPaddingVertical));

	uiWindowsRegisterWM_NOTIFYHandler(t->tabHWND, onWM_NOTIFY, uiControl(t));

	t->pages = new std::vector<struct tabPage *>;
	uiTabOnSelected(t, defaultOnSelected, NULL);

	t->onClosing = NULL;
	t->onClosingData = NULL;
	t->hotCloseButton = -1;
	t->pressedCloseButton = -1;
	t->trackingMouse = FALSE;
	if (SetWindowSubclass(t->tabHWND, tabSubProc, 0, (DWORD_PTR) t) == FALSE)
		logLastError(L"error subclassing uiTab to handle its close buttons");

	return t;
}
