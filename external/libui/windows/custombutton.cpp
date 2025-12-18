// Custom Button - Customizable button with background, hover, pressed colors and flat style
#include "uipriv_windows.hpp"
#include <commctrl.h>
#include <uxtheme.h>
#include <vssym32.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")

struct uiCustomButton {
	uiWindowsControl c;
	HWND hwnd;
	void (*onClicked)(uiCustomButton *, void *);
	void *onClickedData;
	// Custom background color (RGB)
	BOOL hasBackgroundColor;
	COLORREF backgroundColor;
	// Custom pressed color (RGB)
	BOOL hasPressedColor;
	COLORREF pressedColor;
	// Custom hover color (RGB)
	BOOL hasHoverColor;
	COLORREF hoverColor;
	// Track button state
	BOOL isPressed;
	BOOL isHovered;
	// Flat style (square corners, no theme)
	BOOL flatStyle;
	// Border color for flat style
	BOOL hasBorderColor;
	COLORREF borderColor;
};

static BOOL onWM_COMMAND(uiControl *c, HWND hwnd, WORD code, LRESULT *lResult)
{
	uiCustomButton *b = uiCustomButton(c);

	if (code != BN_CLICKED)
		return FALSE;
	(*(b->onClicked))(b, b->onClickedData);
	*lResult = 0;
	return TRUE;
}

// Helper to draw flat style button (square corners)
static void drawFlatButton(HDC hdc, RECT *rc, uiCustomButton *b, COLORREF bgColor, HWND hwnd)
{
	// Fill background with custom color
	HBRUSH hBrush = CreateSolidBrush(bgColor);
	FillRect(hdc, rc, hBrush);
	DeleteObject(hBrush);

	// Draw border
	COLORREF borderColor = b->hasBorderColor ? b->borderColor : RGB(100, 100, 100);
	HPEN hPen = CreatePen(PS_SOLID, 1, borderColor);
	HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
	HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
	Rectangle(hdc, rc->left, rc->top, rc->right, rc->bottom);
	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
	DeleteObject(hPen);

	// Draw text
	WCHAR text[256];
	GetWindowTextW(hwnd, text, 256);

	SetBkMode(hdc, TRANSPARENT);
	if (!IsWindowEnabled(hwnd)) {
		SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));
	} else {
		int r = GetRValue(bgColor);
		int g = GetGValue(bgColor);
		int b_val = GetBValue(bgColor);
		double luminance = (0.299 * r + 0.587 * g + 0.114 * b_val) / 255.0;
		SetTextColor(hdc, luminance > 0.5 ? RGB(0, 0, 0) : RGB(255, 255, 255));
	}

	HFONT hFont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
	HFONT hOldFont = NULL;
	if (hFont)
		hOldFont = (HFONT)SelectObject(hdc, hFont);

	RECT textRect = *rc;
	InflateRect(&textRect, -2, -2);
	DrawTextW(hdc, text, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	if (hOldFont)
		SelectObject(hdc, hOldFont);
}


// Custom button subclass procedure for handling custom colors
static LRESULT CALLBACK customButtonSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	uiCustomButton *b = (uiCustomButton *)dwRefData;

	switch (uMsg) {
	case WM_LBUTTONDOWN:
		b->isPressed = TRUE;
		InvalidateRect(hwnd, NULL, TRUE);
		break;
	case WM_LBUTTONUP:
		b->isPressed = FALSE;
		InvalidateRect(hwnd, NULL, TRUE);
		break;
	case WM_MOUSEMOVE:
		if (!b->isHovered) {
			b->isHovered = TRUE;
			InvalidateRect(hwnd, NULL, TRUE);
			// Request WM_MOUSELEAVE notification
			TRACKMOUSEEVENT tme;
			tme.cbSize = sizeof(tme);
			tme.dwFlags = TME_LEAVE;
			tme.hwndTrack = hwnd;
			TrackMouseEvent(&tme);
		}
		break;
	// Prevent z-order changes when mouse activates the control
	case WM_MOUSEACTIVATE:
		return MA_NOACTIVATE;
	case WM_MOUSELEAVE:
		b->isPressed = FALSE;
		b->isHovered = FALSE;
		InvalidateRect(hwnd, NULL, TRUE);
		break;
	// Prevent focus rectangle from being drawn by the system
	case WM_SETFOCUS:
	case WM_KILLFOCUS:
		InvalidateRect(hwnd, NULL, TRUE);
		return 0;
	// Prevent the default focus cue (orange border on Windows 10/11)
	case WM_UPDATEUISTATE:
		InvalidateRect(hwnd, NULL, TRUE);
		return 0;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			RECT rc;
			GetClientRect(hwnd, &rc);

			// Determine which color to use (priority: pressed > hover > normal)
			COLORREF bgColor;
			if (b->isPressed && b->hasPressedColor) {
				bgColor = b->pressedColor;
			} else if (b->isHovered && b->hasHoverColor) {
				bgColor = b->hoverColor;
			} else if (b->hasBackgroundColor) {
				bgColor = b->backgroundColor;
			} else {
				bgColor = GetSysColor(COLOR_BTNFACE);
			}

			// Use flat style (square corners) if enabled
			if (b->flatStyle) {
				drawFlatButton(hdc, &rc, b, bgColor, hwnd);
				EndPaint(hwnd, &ps);
				return 0;
			}

			// Draw the button with theme border but custom background
			HTHEME hTheme = OpenThemeData(hwnd, L"BUTTON");
			if (hTheme) {
				// Get the button state for theming
				int stateId = PBS_NORMAL;
				if (!IsWindowEnabled(hwnd))
					stateId = PBS_DISABLED;
				else if (b->isPressed)
					stateId = PBS_PRESSED;

				// Draw the themed border/frame
				DrawThemeBackground(hTheme, hdc, BP_PUSHBUTTON, stateId, &rc, NULL);

				// Get content rect (inside the border)
				RECT contentRect;
				GetThemeBackgroundContentRect(hTheme, hdc, BP_PUSHBUTTON, stateId, &rc, &contentRect);

				// Fill the content area with custom color
				HBRUSH hBrush = CreateSolidBrush(bgColor);
				FillRect(hdc, &contentRect, hBrush);
				DeleteObject(hBrush);

				// Draw the button text
				WCHAR text[256];
				GetWindowTextW(hwnd, text, 256);

				// Set text properties
				SetBkMode(hdc, TRANSPARENT);
				if (!IsWindowEnabled(hwnd)) {
					SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));
				} else {
					// Calculate luminance to determine text color
					int r = GetRValue(bgColor);
					int g = GetGValue(bgColor);
					int b_val = GetBValue(bgColor);
					double luminance = (0.299 * r + 0.587 * g + 0.114 * b_val) / 255.0;
					SetTextColor(hdc, luminance > 0.5 ? RGB(0, 0, 0) : RGB(255, 255, 255));
				}

				// Use the button's font
				HFONT hFont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
				HFONT hOldFont = NULL;
				if (hFont)
					hOldFont = (HFONT)SelectObject(hdc, hFont);

				// Draw text centered
				DrawTextW(hdc, text, -1, &contentRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

				if (hOldFont)
					SelectObject(hdc, hOldFont);

				CloseThemeData(hTheme);
			} else {
				// Fallback for non-themed systems - use flat style
				drawFlatButton(hdc, &rc, b, bgColor, hwnd);
			}

			EndPaint(hwnd, &ps);
			return 0;
		}
	case WM_NCDESTROY:
		RemoveWindowSubclass(hwnd, customButtonSubclassProc, uIdSubclass);
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

static void uiCustomButtonDestroy(uiControl *c)
{
	uiCustomButton *b = uiCustomButton(c);

	uiWindowsUnregisterWM_COMMANDHandler(b->hwnd);
	uiWindowsEnsureDestroyWindow(b->hwnd);
	uiFreeControl(uiControl(b));
}

uiWindowsControlAllDefaultsExceptDestroy(uiCustomButton)

#define buttonHeight 14

static void uiCustomButtonMinimumSize(uiWindowsControl *c, int *width, int *height)
{
	uiCustomButton *b = uiCustomButton(c);
	SIZE size;
	uiWindowsSizing sizing;
	int y;

	size.cx = 0;
	size.cy = 0;
	if (SendMessageW(b->hwnd, BCM_GETIDEALSIZE, 0, (LPARAM) (&size)) != FALSE) {
		*width = size.cx;
		*height = size.cy;
		return;
	}

	*width = uiWindowsWindowTextWidth(b->hwnd) + (2 * GetSystemMetrics(SM_CXEDGE));
	y = buttonHeight;
	uiWindowsGetSizing(b->hwnd, &sizing);
	uiWindowsSizingDlgUnitsToPixels(&sizing, NULL, &y);
	*height = y;
}

static void defaultOnClicked(uiCustomButton *b, void *data)
{
	// do nothing
}

char *uiCustomButtonText(uiCustomButton *b)
{
	return uiWindowsWindowText(b->hwnd);
}

void uiCustomButtonSetText(uiCustomButton *b, const char *text)
{
	uiWindowsSetWindowText(b->hwnd, text);
	uiWindowsControlMinimumSizeChanged(uiWindowsControl(b));
}

void uiCustomButtonOnClicked(uiCustomButton *b, void (*f)(uiCustomButton *, void *), void *data)
{
	b->onClicked = f;
	b->onClickedData = data;
}


uiCustomButton *uiNewCustomButton(const char *text)
{
	uiCustomButton *b;
	WCHAR *wtext;

	uiWindowsNewControl(uiCustomButton, b);

	wtext = toUTF16(text);
	b->hwnd = uiWindowsEnsureCreateControlHWND(0,
		L"button", wtext,
		BS_PUSHBUTTON | WS_TABSTOP,
		hInstance, NULL,
		TRUE);
	uiprivFree(wtext);

	// Initialize custom color fields
	b->hasBackgroundColor = FALSE;
	b->backgroundColor = RGB(255, 255, 255);
	b->hasPressedColor = FALSE;
	b->pressedColor = RGB(200, 200, 200);
	b->hasHoverColor = FALSE;
	b->hoverColor = RGB(230, 230, 230);
	b->isPressed = FALSE;
	b->isHovered = FALSE;
	b->flatStyle = FALSE;
	b->hasBorderColor = FALSE;
	b->borderColor = RGB(100, 100, 100);

	// Install subclass for custom painting
	SetWindowSubclass(b->hwnd, customButtonSubclassProc, 0, (DWORD_PTR)b);

	uiWindowsRegisterWM_COMMANDHandler(b->hwnd, onWM_COMMAND, uiControl(b));
	uiCustomButtonOnClicked(b, defaultOnClicked, NULL);

	return b;
}

void uiCustomButtonSetBackgroundColor(uiCustomButton *b, int r, int g, int bl)
{
	b->hasBackgroundColor = TRUE;
	b->backgroundColor = RGB(r, g, bl);
	InvalidateRect(b->hwnd, NULL, TRUE);
}

void uiCustomButtonSetPressedColor(uiCustomButton *b, int r, int g, int bl)
{
	b->hasPressedColor = TRUE;
	b->pressedColor = RGB(r, g, bl);
	InvalidateRect(b->hwnd, NULL, TRUE);
}

void uiCustomButtonClearBackgroundColor(uiCustomButton *b)
{
	b->hasBackgroundColor = FALSE;
	InvalidateRect(b->hwnd, NULL, TRUE);
}

void uiCustomButtonClearPressedColor(uiCustomButton *b)
{
	b->hasPressedColor = FALSE;
	InvalidateRect(b->hwnd, NULL, TRUE);
}

void uiCustomButtonSetFlat(uiCustomButton *b, int flat)
{
	b->flatStyle = flat ? TRUE : FALSE;
	InvalidateRect(b->hwnd, NULL, TRUE);
}

void uiCustomButtonSetBorderColor(uiCustomButton *b, int r, int g, int bl)
{
	b->hasBorderColor = TRUE;
	b->borderColor = RGB(r, g, bl);
	InvalidateRect(b->hwnd, NULL, TRUE);
}

void uiCustomButtonClearBorderColor(uiCustomButton *b)
{
	b->hasBorderColor = FALSE;
	InvalidateRect(b->hwnd, NULL, TRUE);
}

void uiCustomButtonSetHoverColor(uiCustomButton *b, int r, int g, int bl)
{
	b->hasHoverColor = TRUE;
	b->hoverColor = RGB(r, g, bl);
	InvalidateRect(b->hwnd, NULL, TRUE);
}

void uiCustomButtonClearHoverColor(uiCustomButton *b)
{
	b->hasHoverColor = FALSE;
	InvalidateRect(b->hwnd, NULL, TRUE);
}
