// uiMarkdownViewer - A read-only control that renders a basic subset of markdown
#include "uipriv_windows.hpp"
#include "markdown.hpp"
#include <shellapi.h>

#define markdownViewerPaddingX 10
#define markdownViewerPaddingY 8
#define markdownViewerIndentStep 18
#define markdownViewerMarkerGap 5
#define markdownViewerFontCount (markdownBlockKindCount * markdownStyleCount)
#define markdownViewerFallbackBorder RGB(0xAC, 0xAC, 0xAC)
#define markdownViewerCodeFace L"Consolas"
#define markdownViewerCodeScale 92
#define markdownViewerCodePadding 2
#define markdownViewerLinkColor RGB(0, 102, 204)

static const int blockFontScales[markdownBlockKindCount] = { 100, 100, 100, 175, 140, 120, 105 };
static const int blockSpaceBefore[markdownBlockKindCount] = { 4, 0, 0, 8, 8, 6, 5 };
static const int blockSpaceAfter[markdownBlockKindCount] = { 4, 2, 2, 3, 3, 2, 2 };

struct viewerFragment {
	std::wstring text;
	std::wstring url;
	markdownBlockKind kind;
	int style;
	int x;
	int width;
	int ascent;
	BOOL mergeable;
};

struct viewerLine {
	std::vector<viewerFragment> fragments;
	int y;
	int ascent;
	int descent;
	int height;
};

struct uiMarkdownViewer {
	uiWindowsControl c;
	HWND hwnd;
	std::vector<markdownBlock> *blocks;
	std::vector<viewerLine> *lines;
	HFONT fonts[markdownViewerFontCount];
	int contentHeight;
	int lineStep;
	int scrollPos;
};

static HFONT viewerFont(uiMarkdownViewer *mv, markdownBlockKind kind, int style)
{
	int index = kind * markdownStyleCount + style;
	LOGFONTW lf;

	if (mv->fonts[index] != NULL)
		return mv->fonts[index];

	GetObject(hMessageFont, sizeof (LOGFONTW), &lf);
	lf.lfHeight = MulDiv(lf.lfHeight, blockFontScales[kind], 100);
	if ((style & markdownStyleBold) != 0 || kind >= markdownHeading1)
		lf.lfWeight = FW_BOLD;
	lf.lfItalic = (style & markdownStyleItalic) != 0;
	lf.lfUnderline = (style & (markdownStyleUnderline | markdownStyleLink)) != 0;
	if ((style & markdownStyleCode) != 0) {
		lf.lfHeight = MulDiv(lf.lfHeight, markdownViewerCodeScale, 100);
		lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
		wcscpy_s(lf.lfFaceName, LF_FACESIZE, markdownViewerCodeFace);
	}

	mv->fonts[index] = CreateFontIndirectW(&lf);
	return mv->fonts[index];
}

static void appendFragment(viewerLine *line, const markdownBlock &block, int style, const std::wstring &url, const std::wstring &text, int x, int width, int ascent, BOOL mergeable)
{
	viewerFragment fragment;

	fragment.text = text;
	fragment.url = url;
	fragment.kind = block.kind;
	fragment.style = style;
	fragment.x = x;
	fragment.width = width;
	fragment.ascent = ascent;
	fragment.mergeable = mergeable;
	line->fragments.push_back(fragment);
}

static void beginLine(viewerLine *line, int y)
{
	line->fragments.clear();
	line->y = y;
	line->ascent = 0;
	line->descent = 0;
	line->height = 0;
}

// mixed font sizes on one line share a baseline, so the line grows on both sides of it
static void growLine(viewerLine *line, const TEXTMETRICW *tm)
{
	if (tm->tmAscent > line->ascent)
		line->ascent = tm->tmAscent;
	if (tm->tmDescent > line->descent)
		line->descent = tm->tmDescent;
	line->height = line->ascent + line->descent;
}

// only text of the same style that directly follows the previous word can grow a
// fragment; spaces between fragments are left undrawn so they carry no underline
static BOOL canMergeInto(const viewerLine *line, const markdownRun &run)
{
	if (line->fragments.empty())
		return FALSE;
	if (!line->fragments.back().mergeable || line->fragments.back().style != run.style)
		return FALSE;
	return line->fragments.back().url == run.url;
}

static int layoutBlock(uiMarkdownViewer *mv, HDC dc, const markdownBlock &block, int width, int top)
{
	int marginLeft = block.indent * markdownViewerIndentStep;
	int indent = marginLeft;
	int x, y = top;
	viewerLine line;
	BOOL pendingSpace = FALSE;

	beginLine(&line, y);

	if (!block.marker.empty()) {
		TEXTMETRICW tm;
		SIZE marker;

		SelectObject(dc, viewerFont(mv, block.kind, 0));
		GetTextMetricsW(dc, &tm);
		GetTextExtentPoint32W(dc, block.marker.c_str(), (int) block.marker.length(), &marker);
		growLine(&line, &tm);
		appendFragment(&line, block, 0, L"", block.marker, marginLeft, marker.cx, tm.tmAscent, FALSE);
		indent = marginLeft + marker.cx + markdownViewerMarkerGap;
	}
	x = indent;

	for (size_t r = 0; r < block.runs.size(); r++) {
		const markdownRun &run = block.runs[r];
		TEXTMETRICW tm;
		SIZE space;
		size_t i = 0;

		SelectObject(dc, viewerFont(mv, block.kind, run.style));
		GetTextMetricsW(dc, &tm);
		GetTextExtentPoint32W(dc, L" ", 1, &space);
		growLine(&line, &tm);

		while (i < run.text.length()) {
			if (run.text[i] == L' ') {
				pendingSpace = TRUE;
				i++;
				continue;
			}

			size_t end = run.text.find(L' ', i);
			if (end == std::wstring::npos)
				end = run.text.length();

			std::wstring word = run.text.substr(i, end - i);
			SIZE size;
			int gap;

			GetTextExtentPoint32W(dc, word.c_str(), (int) word.length(), &size);
			gap = (pendingSpace && x > indent) ? space.cx : 0;

			if (x + gap + size.cx > width && x > indent) {
				mv->lines->push_back(line);
				y += line.height;
				beginLine(&line, y);
				growLine(&line, &tm);
				x = indent;
				gap = 0;
			}

			if (canMergeInto(&line, run)) {
				viewerFragment &last = line.fragments.back();

				if (gap > 0)
					last.text += L' ';
				last.text += word;
				last.width += gap + size.cx;
			} else {
				x += gap;
				gap = 0;
				appendFragment(&line, block, run.style, run.url, word, x, size.cx, tm.tmAscent, TRUE);
			}

			x += gap + size.cx;
			pendingSpace = FALSE;
			i = end;
		}
	}

	if (!line.fragments.empty()) {
		mv->lines->push_back(line);
		y += line.height;
	}
	return y;
}

static void relayout(uiMarkdownViewer *mv)
{
	RECT client;
	HDC dc;
	TEXTMETRICW tm;
	int width, y;

	uiWindowsEnsureGetClientRect(mv->hwnd, &client);
	width = (client.right - client.left) - 2 * markdownViewerPaddingX;
	if (width < 1)
		width = 1;

	mv->lines->clear();
	dc = GetDC(mv->hwnd);

	SelectObject(dc, viewerFont(mv, markdownParagraph, 0));
	GetTextMetricsW(dc, &tm);
	mv->lineStep = tm.tmHeight;

	y = markdownViewerPaddingY;
	for (size_t i = 0; i < mv->blocks->size(); i++) {
		const markdownBlock &block = (*(mv->blocks))[i];

		if (i > 0)
			y += blockSpaceBefore[block.kind];
		y = layoutBlock(mv, dc, block, width, y);
		y += blockSpaceAfter[block.kind];
	}

	ReleaseDC(mv->hwnd, dc);
	mv->contentHeight = y + markdownViewerPaddingY;
}

static void updateScrollInfo(uiMarkdownViewer *mv)
{
	RECT client;
	SCROLLINFO si;
	int page, maxPos;

	uiWindowsEnsureGetClientRect(mv->hwnd, &client);
	page = client.bottom - client.top;
	maxPos = mv->contentHeight - page;
	if (maxPos < 0)
		maxPos = 0;
	if (mv->scrollPos > maxPos)
		mv->scrollPos = maxPos;

	ZeroMemory(&si, sizeof (SCROLLINFO));
	si.cbSize = sizeof (SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS | SIF_DISABLENOSCROLL;
	si.nMin = 0;
	si.nMax = mv->contentHeight - 1;
	si.nPage = page;
	si.nPos = mv->scrollPos;
	SetScrollInfo(mv->hwnd, SB_VERT, &si, TRUE);
}

static void scrollTo(uiMarkdownViewer *mv, int pos)
{
	RECT client;
	SCROLLINFO si;
	int maxPos;

	uiWindowsEnsureGetClientRect(mv->hwnd, &client);
	maxPos = mv->contentHeight - (client.bottom - client.top);
	if (maxPos < 0)
		maxPos = 0;
	if (pos > maxPos)
		pos = maxPos;
	if (pos < 0)
		pos = 0;
	if (pos == mv->scrollPos)
		return;

	mv->scrollPos = pos;

	ZeroMemory(&si, sizeof (SCROLLINFO));
	si.cbSize = sizeof (SCROLLINFO);
	si.fMask = SIF_POS;
	si.nPos = mv->scrollPos;
	SetScrollInfo(mv->hwnd, SB_VERT, &si, TRUE);

	InvalidateRect(mv->hwnd, NULL, FALSE);
}

// the WS_BORDER frame is unthemed, so it gets repainted with the color themed edits and tables use
static void paintBorder(HWND hwnd)
{
	COLORREF color = markdownViewerFallbackBorder;
	HTHEME theme = OpenThemeData(hwnd, L"Edit");
	HBRUSH brush;
	HDC dc;
	RECT r;

	if (theme != NULL) {
		if (FAILED(GetThemeColor(theme, EP_EDITBORDER_NOSCROLL, EPSN_NORMAL, TMT_BORDERCOLOR, &color)))
			color = markdownViewerFallbackBorder;
		CloseThemeData(theme);
	}

	uiWindowsEnsureGetWindowRect(hwnd, &r);
	OffsetRect(&r, -r.left, -r.top);

	dc = GetWindowDC(hwnd);
	brush = CreateSolidBrush(color);
	FrameRect(dc, &r, brush);
	DeleteObject(brush);
	ReleaseDC(hwnd, dc);
}

// the padding bleeds into the space gap left undrawn around the fragment
static void paintCodeBackground(HDC dc, const std::wstring &text, int x, int y)
{
	SIZE size;
	RECT r;

	GetTextExtentPoint32W(dc, text.c_str(), (int) text.length(), &size);
	r.left = x - markdownViewerCodePadding;
	r.top = y;
	r.right = x + size.cx + markdownViewerCodePadding;
	r.bottom = y + size.cy;
	FillRect(dc, &r, GetSysColorBrush(COLOR_BTNFACE));
}

static void paint(uiMarkdownViewer *mv, HDC dc, RECT *client)
{
	FillRect(dc, client, GetSysColorBrush(COLOR_WINDOW));
	SetBkMode(dc, TRANSPARENT);

	for (size_t i = 0; i < mv->lines->size(); i++) {
		const viewerLine &line = (*(mv->lines))[i];
		int y = line.y - mv->scrollPos;

		if (y + line.height < client->top || y > client->bottom)
			continue;
		for (size_t f = 0; f < line.fragments.size(); f++) {
			const viewerFragment &fragment = line.fragments[f];

			int x = markdownViewerPaddingX + fragment.x;
			int top = y + line.ascent - fragment.ascent;

			SelectObject(dc, viewerFont(mv, fragment.kind, fragment.style));
			if ((fragment.style & markdownStyleLink) != 0)
				SetTextColor(dc, markdownViewerLinkColor);
			else
				SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
			if ((fragment.style & markdownStyleCode) != 0)
				paintCodeBackground(dc, fragment.text, x, top);
			TextOutW(dc, x, top, fragment.text.c_str(), (int) fragment.text.length());
		}
	}
}

static const std::wstring *linkURLAt(uiMarkdownViewer *mv, int x, int y)
{
	for (size_t i = 0; i < mv->lines->size(); i++) {
		const viewerLine &line = (*(mv->lines))[i];
		int top = line.y - mv->scrollPos;

		if (y < top || y >= top + line.height)
			continue;
		for (size_t f = 0; f < line.fragments.size(); f++) {
			const viewerFragment &fragment = line.fragments[f];
			int left = markdownViewerPaddingX + fragment.x;

			if ((fragment.style & markdownStyleLink) == 0)
				continue;
			if (x >= left && x < left + fragment.width)
				return &(fragment.url);
		}
	}
	return NULL;
}

static void paintBuffered(uiMarkdownViewer *mv)
{
	PAINTSTRUCT ps;
	RECT client;
	HDC dc, bufferDC;
	HBITMAP buffer;
	HGDIOBJ oldBitmap;

	uiWindowsEnsureGetClientRect(mv->hwnd, &client);
	dc = BeginPaint(mv->hwnd, &ps);

	bufferDC = CreateCompatibleDC(dc);
	buffer = CreateCompatibleBitmap(dc, client.right, client.bottom);
	oldBitmap = SelectObject(bufferDC, buffer);

	paint(mv, bufferDC, &client);
	BitBlt(dc, 0, 0, client.right, client.bottom, bufferDC, 0, 0, SRCCOPY);

	SelectObject(bufferDC, oldBitmap);
	DeleteObject(buffer);
	DeleteDC(bufferDC);

	EndPaint(mv->hwnd, &ps);
}

static LRESULT CALLBACK markdownViewerSubProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	uiMarkdownViewer *mv = (uiMarkdownViewer *) dwRefData;

	switch (uMsg) {
	case WM_PAINT:
		paintBuffered(mv);
		return 0;
	case WM_ERASEBKGND:
		return 1;
	case WM_NCPAINT:
		{
			LRESULT lResult = DefSubclassProc(hwnd, uMsg, wParam, lParam);
			paintBorder(hwnd);
			return lResult;
		}
	case WM_SIZE:
		relayout(mv);
		updateScrollInfo(mv);
		InvalidateRect(hwnd, NULL, FALSE);
		break;
	case WM_VSCROLL:
		{
			SCROLLINFO si;
			int pos = mv->scrollPos;

			ZeroMemory(&si, sizeof (SCROLLINFO));
			si.cbSize = sizeof (SCROLLINFO);
			si.fMask = SIF_PAGE | SIF_TRACKPOS;
			GetScrollInfo(hwnd, SB_VERT, &si);

			switch (LOWORD(wParam)) {
			case SB_TOP:
				pos = 0;
				break;
			case SB_BOTTOM:
				pos = mv->contentHeight;
				break;
			case SB_LINEUP:
				pos -= mv->lineStep;
				break;
			case SB_LINEDOWN:
				pos += mv->lineStep;
				break;
			case SB_PAGEUP:
				pos -= (int) si.nPage;
				break;
			case SB_PAGEDOWN:
				pos += (int) si.nPage;
				break;
			case SB_THUMBTRACK:
			case SB_THUMBPOSITION:
				pos = si.nTrackPos;
				break;
			}
			scrollTo(mv, pos);
		}
		return 0;
	case WM_MOUSEWHEEL:
		scrollTo(mv, mv->scrollPos - (GET_WHEEL_DELTA_WPARAM(wParam) * 3 * mv->lineStep) / WHEEL_DELTA);
		return 0;
	case WM_LBUTTONDOWN:
		// the wheel only reaches the control that holds the focus
		SetFocus(hwnd);
		return 0;
	case WM_LBUTTONUP:
		{
			const std::wstring *url = linkURLAt(mv, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

			if (url != NULL)
				ShellExecuteW(NULL, L"open", url->c_str(), NULL, NULL, SW_SHOWNORMAL);
		}
		return 0;
	case WM_SETCURSOR:
		{
			POINT pt;

			GetCursorPos(&pt);
			ScreenToClient(hwnd, &pt);
			if (linkURLAt(mv, pt.x, pt.y) != NULL) {
				SetCursor(LoadCursorW(NULL, IDC_HAND));
				return TRUE;
			}
		}
		break;
	case WM_NCDESTROY:
		RemoveWindowSubclass(hwnd, markdownViewerSubProc, uIdSubclass);
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

static void uiMarkdownViewerDestroy(uiControl *c)
{
	uiMarkdownViewer *mv = uiMarkdownViewer(c);
	int i;

	for (i = 0; i < markdownViewerFontCount; i++)
		if (mv->fonts[i] != NULL)
			DeleteObject(mv->fonts[i]);
	delete mv->lines;
	delete mv->blocks;

	uiWindowsEnsureDestroyWindow(mv->hwnd);
	uiFreeControl(c);
}

uiWindowsControlAllDefaultsExceptDestroy(uiMarkdownViewer)

#define markdownViewerWidth 107
#define markdownViewerHeight 42

static void uiMarkdownViewerMinimumSize(uiWindowsControl *c, int *width, int *height)
{
	uiMarkdownViewer *mv = uiMarkdownViewer(c);
	uiWindowsSizing sizing;
	int x, y;

	x = markdownViewerWidth;
	y = markdownViewerHeight;
	uiWindowsGetSizing(mv->hwnd, &sizing);
	uiWindowsSizingDlgUnitsToPixels(&sizing, &x, &y);
	*width = x;
	*height = y;
}

void uiMarkdownViewerSetText(uiMarkdownViewer *mv, const char *markdown)
{
	uiprivMarkdownParse(markdown, mv->blocks);
	mv->scrollPos = 0;
	relayout(mv);
	updateScrollInfo(mv);
	InvalidateRect(mv->hwnd, NULL, FALSE);
}

uiMarkdownViewer *uiNewMarkdownViewer(void)
{
	uiMarkdownViewer *mv;

	uiWindowsNewControl(uiMarkdownViewer, mv);

	mv->blocks = new std::vector<markdownBlock>;
	mv->lines = new std::vector<viewerLine>;
	ZeroMemory(mv->fonts, sizeof (mv->fonts));
	mv->lineStep = 1;

	mv->hwnd = uiWindowsEnsureCreateControlHWND(0,
		L"static", L"",
		// without SS_NOTIFY a static is hit-test transparent, so neither the
		// scrollbar nor the wheel would react
		SS_OWNERDRAW | SS_NOTIFY | WS_BORDER | WS_VSCROLL,
		hInstance, NULL,
		TRUE);

	SetWindowSubclass(mv->hwnd, markdownViewerSubProc, 0, (DWORD_PTR) mv);

	return mv;
}
