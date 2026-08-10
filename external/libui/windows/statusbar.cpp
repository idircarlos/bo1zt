// uiStatusBar - Status bar whose items are laid out in append order and can carry text and icons
#include "uipriv_windows.hpp"

#define statusBarItemPadding 6
#define statusBarIconGap 4
#define statusBarVerticalPadding 4
#define statusBarIconMaskSamples 4

struct uiprivStatusBarItem {
	WCHAR *text;
	uiImage *image;
	HBITMAP iconBitmap;
	BOOL circularIcon;
	int width;
	int minWidth;
	int maxWidth;
};

struct uiStatusBar {
	HWND hwnd;
	int height;
	BOOL grip;
	std::vector<uiprivStatusBarItem *> *items;
};

static uiprivStatusBarItem *statusBarItem(uiStatusBar *sb, int item)
{
	if (item < 0 || (size_t) item >= sb->items->size())
		uiprivUserBug("Item %d is out of range for this uiStatusBar.", item);
	return (*(sb->items))[item];
}

static int statusBarTextWidth(uiStatusBar *sb, const WCHAR *text)
{
	HDC dc;
	HFONT font, prevFont;
	SIZE size;

	dc = GetDC(sb->hwnd);
	font = (HFONT) SendMessageW(sb->hwnd, WM_GETFONT, 0, 0);
	prevFont = (HFONT) SelectObject(dc, font);
	if (GetTextExtentPoint32W(dc, text, (int) wcslen(text), &size) == 0) {
		logLastError(L"error measuring uiStatusBar item text");
		size.cx = 0;
	}
	SelectObject(dc, prevFont);
	ReleaseDC(sb->hwnd, dc);
	return size.cx;
}

// SB_GETRECT insets each part by the control borders, so the requested width has to make room for them
static int statusBarPartOverhead(uiStatusBar *sb)
{
	int borders[3];

	if (SendMessageW(sb->hwnd, SB_GETBORDERS, 0, (LPARAM) borders) == 0)
		return 0;
	return 2 * borders[0] + borders[2];
}

static int statusBarItemWidth(uiStatusBar *sb, uiprivStatusBarItem *item)
{
	int width;

	width = 2 * statusBarItemPadding + statusBarTextWidth(sb, item->text) + statusBarPartOverhead(sb);
	if (item->image != NULL)
		width += GetSystemMetrics(SM_CXSMICON) + statusBarIconGap;
	if (item->maxWidth >= 0 && width > item->maxWidth)
		width = item->maxWidth;
	if (width < item->minWidth)
		width = item->minWidth;
	return width;
}

static void statusBarSetParts(uiStatusBar *sb)
{
	std::vector<int> edges;
	int edge = 0;

	for (uiprivStatusBarItem *item : *(sb->items)) {
		item->width = statusBarItemWidth(sb, item);
		edge += item->width;
		edges.push_back(edge);
	}
	if (edges.empty())
		edges.push_back(-1);
	else
		edges[edges.size() - 1] = -1;
	SendMessageW(sb->hwnd, SB_SETPARTS, (WPARAM) (edges.size()), (LPARAM) (edges.data()));
	invalidateRect(sb->hwnd, NULL, FALSE);
}

static void statusBarItemChanged(uiStatusBar *sb, int item)
{
	uiprivStatusBarItem *i = (*(sb->items))[item];
	RECT part;

	if (statusBarItemWidth(sb, i) != i->width) {
		statusBarSetParts(sb);
		return;
	}
	if (SendMessageW(sb->hwnd, SB_GETRECT, (WPARAM) item, (LPARAM) (&part)) != 0)
		invalidateRect(sb->hwnd, &part, FALSE);
}

static void statusBarItemClearIcon(uiprivStatusBarItem *item)
{
	if (item->iconBitmap != NULL) {
		DeleteObject(item->iconBitmap);
		item->iconBitmap = NULL;
	}
	item->image = NULL;
}

static void statusBarFreeItem(uiprivStatusBarItem *item)
{
	statusBarItemClearIcon(item);
	uiprivFree(item->text);
	uiprivFree(item);
}

static double statusBarCircleCoverage(int x, int y, double centerX, double centerY, double radius)
{
	int inside = 0;
	int sx, sy;

	for (sy = 0; sy < statusBarIconMaskSamples; sy++)
		for (sx = 0; sx < statusBarIconMaskSamples; sx++) {
			double dx = x + (sx + 0.5) / statusBarIconMaskSamples - centerX;
			double dy = y + (sy + 0.5) / statusBarIconMaskSamples - centerY;
			if (dx * dx + dy * dy <= radius * radius)
				inside++;
		}
	return (double) inside / (statusBarIconMaskSamples * statusBarIconMaskSamples);
}

// the bitmap holds premultiplied BGRA, so scaling every channel by the coverage is enough to mask it
static void statusBarMaskIconCircle(HBITMAP bitmap)
{
	BITMAP bmp;
	double centerX, centerY, radius;
	int x, y, channel;

	if (GetObject(bitmap, sizeof (BITMAP), &bmp) == 0 || bmp.bmBits == NULL)
		return;
	centerX = bmp.bmWidth / 2.0;
	centerY = bmp.bmHeight / 2.0;
	radius = centerX < centerY ? centerX : centerY;
	for (y = 0; y < bmp.bmHeight; y++) {
		BYTE *row = (BYTE *) (bmp.bmBits) + (size_t) y * bmp.bmWidthBytes;

		for (x = 0; x < bmp.bmWidth; x++) {
			BYTE *pixel = row + 4 * x;
			double coverage = statusBarCircleCoverage(x, y, centerX, centerY, radius);

			if (coverage >= 1.0)
				continue;
			for (channel = 0; channel < 4; channel++)
				pixel[channel] = (BYTE) (pixel[channel] * coverage + 0.5);
		}
	}
}

static HBITMAP statusBarItemIconBitmap(uiprivStatusBarItem *item, HDC dc)
{
	IWICBitmap *source;

	if (item->iconBitmap != NULL)
		return item->iconBitmap;
	if (item->image == NULL)
		return NULL;
	source = uiprivImageAppropriateForDC(item->image, dc);
	if (source == NULL)
		return NULL;
	if (uiprivWICToGDI(source, dc, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), &(item->iconBitmap)) != S_OK)
		item->iconBitmap = NULL;
	else if (item->circularIcon)
		statusBarMaskIconCircle(item->iconBitmap);
	return item->iconBitmap;
}

static void statusBarDrawIcon(HDC dc, HBITMAP bitmap, int x, int y)
{
	HDC memDC;
	HGDIOBJ prevBitmap;
	BLENDFUNCTION bf;
	int width, height;

	width = GetSystemMetrics(SM_CXSMICON);
	height = GetSystemMetrics(SM_CYSMICON);
	memDC = CreateCompatibleDC(dc);
	prevBitmap = SelectObject(memDC, bitmap);
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.SourceConstantAlpha = 255;
	bf.AlphaFormat = AC_SRC_ALPHA;
	AlphaBlend(dc, x, y, width, height, memDC, 0, 0, width, height, bf);
	SelectObject(memDC, prevBitmap);
	DeleteDC(memDC);
}

static void statusBarDrawItem(uiStatusBar *sb, HDC dc, uiprivStatusBarItem *item, RECT *part)
{
	HBITMAP icon;
	HFONT font, prevFont;
	RECT r;

	r = *part;
	r.left += statusBarItemPadding;
	r.right -= statusBarItemPadding;
	if (item->image != NULL) {
		icon = statusBarItemIconBitmap(item, dc);
		if (icon != NULL)
			statusBarDrawIcon(dc, icon,
				r.left,
				r.top + ((r.bottom - r.top) - GetSystemMetrics(SM_CYSMICON)) / 2);
		r.left += GetSystemMetrics(SM_CXSMICON) + statusBarIconGap;
	}

	SetBkMode(dc, TRANSPARENT);
	SetTextColor(dc, GetSysColor(COLOR_BTNTEXT));
	font = (HFONT) SendMessageW(sb->hwnd, WM_GETFONT, 0, 0);
	prevFont = (HFONT) SelectObject(dc, font);
	DrawTextW(dc, item->text, -1, &r, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
	SelectObject(dc, prevFont);
}

static void statusBarDraw(uiStatusBar *sb, HDC dc, RECT *client)
{
	HTHEME theme;
	RECT part;
	size_t i;

	theme = OpenThemeData(sb->hwnd, L"STATUS");
	if (theme != NULL)
		DrawThemeBackground(theme, dc, 0, 0, client, NULL);
	else
		FillRect(dc, client, GetSysColorBrush(COLOR_BTNFACE));

	for (i = 0; i < sb->items->size(); i++) {
		if (SendMessageW(sb->hwnd, SB_GETRECT, (WPARAM) i, (LPARAM) (&part)) == 0)
			continue;
		// the control always keeps room for a sizing grip, even when we suppress it
		if (i == sb->items->size() - 1 && !sb->grip)
			part.right = client->right;
		if (theme != NULL)
			DrawThemeBackground(theme, dc, SP_PANE, 0, &part, NULL);
		else
			DrawEdge(dc, &part, BDR_SUNKENOUTER, BF_RECT);
		statusBarDrawItem(sb, dc, (*(sb->items))[i], &part);
	}

	if (sb->grip) {
		part = *client;
		part.left = part.right - GetSystemMetrics(SM_CXVSCROLL);
		if (theme != NULL)
			DrawThemeBackground(theme, dc, SP_GRIPPER, 0, &part, NULL);
		else
			DrawFrameControl(dc, &part, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
	}

	if (theme != NULL)
		CloseThemeData(theme);
}

static void statusBarPaint(uiStatusBar *sb, HDC dc, RECT *client)
{
	HDC memDC;
	HBITMAP buffer;
	HGDIOBJ prevBitmap;

	memDC = CreateCompatibleDC(dc);
	buffer = CreateCompatibleBitmap(dc, client->right, client->bottom);
	prevBitmap = SelectObject(memDC, buffer);
	statusBarDraw(sb, memDC, client);
	BitBlt(dc, 0, 0, client->right, client->bottom, memDC, 0, 0, SRCCOPY);
	SelectObject(memDC, prevBitmap);
	DeleteObject(buffer);
	DeleteDC(memDC);
}

static LRESULT CALLBACK statusBarSubProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	uiStatusBar *sb = (uiStatusBar *) dwRefData;
	PAINTSTRUCT ps;
	RECT client;
	HDC dc;

	switch (uMsg) {
	// comctl32 v6 claims the bottom right corner as a sizing grip even without SBARS_SIZEGRIP
	case WM_NCHITTEST:
		if (sb->grip)
			break;
		return HTCLIENT;
	case WM_ERASEBKGND:
		return 1;
	case WM_PAINT:
		dc = BeginPaint(hwnd, &ps);
		GetClientRect(hwnd, &client);
		statusBarPaint(sb, dc, &client);
		EndPaint(hwnd, &ps);
		return 0;
	case WM_NCDESTROY:
		RemoveWindowSubclass(hwnd, statusBarSubProc, uIdSubclass);
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void uiprivStatusBarSetParent(uiStatusBar *sb, HWND parent)
{
	if (parent == NULL)
		parent = utilWindow;
	if (SetParent(sb->hwnd, parent) == NULL)
		logLastError(L"error changing uiStatusBar parent");
}

int uiprivStatusBarHeight(uiStatusBar *sb)
{
	return sb->height;
}

int uiprivStatusBarRelayout(uiStatusBar *sb)
{
	RECT r;

	SendMessageW(sb->hwnd, WM_SIZE, 0, 0);
	uiWindowsEnsureGetWindowRect(sb->hwnd, &r);
	sb->height = r.bottom - r.top;
	return sb->height;
}

void uiprivStatusBarDestroy(uiStatusBar *sb)
{
	for (uiprivStatusBarItem *item : *(sb->items))
		statusBarFreeItem(item);
	delete sb->items;
	uiWindowsEnsureDestroyWindow(sb->hwnd);
	uiprivFree(sb);
}

uiStatusBar *uiNewStatusBar(int grip)
{
	uiStatusBar *sb;

	sb = uiprivNew(uiStatusBar);
	sb->grip = grip ? TRUE : FALSE;
	sb->items = new std::vector<uiprivStatusBarItem *>;

	sb->hwnd = uiWindowsEnsureCreateControlHWND(0,
		STATUSCLASSNAMEW, L"",
		sb->grip ? SBARS_SIZEGRIP : 0,
		hInstance, NULL,
		TRUE);
	SendMessageW(sb->hwnd, SB_SETMINHEIGHT,
		(WPARAM) (GetSystemMetrics(SM_CYSMICON) + statusBarVerticalPadding), 0);
	SetWindowSubclass(sb->hwnd, statusBarSubProc, 0, (DWORD_PTR) sb);
	uiprivStatusBarRelayout(sb);
	statusBarSetParts(sb);

	return sb;
}

int uiStatusBarAppend(uiStatusBar *sb, const char *text)
{
	uiprivStatusBarItem *item;

	item = uiprivNew(uiprivStatusBarItem);
	item->text = toUTF16(text);
	item->maxWidth = -1;
	sb->items->push_back(item);
	statusBarSetParts(sb);
	return (int) (sb->items->size() - 1);
}

void uiStatusBarClear(uiStatusBar *sb)
{
	for (uiprivStatusBarItem *item : *(sb->items))
		statusBarFreeItem(item);
	sb->items->clear();
	statusBarSetParts(sb);
}

char *uiStatusBarItemText(uiStatusBar *sb, int item)
{
	return toUTF8(statusBarItem(sb, item)->text);
}

void uiStatusBarSetItemText(uiStatusBar *sb, int item, const char *text)
{
	uiprivStatusBarItem *i = statusBarItem(sb, item);
	WCHAR *wtext;

	wtext = toUTF16(text);
	if (wcscmp(i->text, wtext) == 0) {
		uiprivFree(wtext);
		return;
	}
	uiprivFree(i->text);
	i->text = wtext;
	statusBarItemChanged(sb, item);
}

void uiStatusBarSetItemWidth(uiStatusBar *sb, int item, int min, int max)
{
	uiprivStatusBarItem *i = statusBarItem(sb, item);

	i->minWidth = min < 0 ? 0 : min;
	i->maxWidth = max;
	statusBarSetParts(sb);
}

void uiStatusBarSetItemImage(uiStatusBar *sb, int item, uiImage *image, int circular)
{
	uiprivStatusBarItem *i = statusBarItem(sb, item);

	statusBarItemClearIcon(i);
	i->image = image;
	i->circularIcon = circular ? TRUE : FALSE;
	statusBarSetParts(sb);
}

void uiStatusBarClearItemIcon(uiStatusBar *sb, int item)
{
	statusBarItemClearIcon(statusBarItem(sb, item));
	statusBarSetParts(sb);
}
