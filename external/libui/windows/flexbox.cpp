// flexbox.cpp - uiFlexBox implementation for absolute positioning
#include "uipriv_windows.hpp"

struct flexChild {
	uiControl *c;
	int x;
	int y;
	int width;
	int height;
};

struct uiFlexBox {
	uiWindowsControl c;
	HWND hwnd;
	std::vector<struct flexChild> *controls;
};

static void flexboxArrangeChildren(uiFlexBox *f);

static void flexboxRelayout(uiFlexBox *f)
{
	if (f->controls->empty())
		return;

	for (const struct flexChild &fc : *(f->controls)) {
		if (!uiControlVisible(fc.c))
			continue;
		uiWindowsEnsureMoveWindowDuringResize(
			(HWND)uiControlHandle(fc.c),
			fc.x,
			fc.y,
			fc.width,
			fc.height);
	}

	// Reafirmar z-order para evitar cambios por eventos de ratón
	flexboxArrangeChildren(f);
}

static void uiFlexBoxDestroy(uiControl *c)
{
	uiFlexBox *f = uiFlexBox(c);

	for (const struct flexChild &fc : *(f->controls)) {
		uiControlSetParent(fc.c, NULL);
		uiControlDestroy(fc.c);
	}
	delete f->controls;
	uiWindowsEnsureDestroyWindow(f->hwnd);
	uiFreeControl(uiControl(f));
}

uiWindowsControlDefaultHandle(uiFlexBox)
uiWindowsControlDefaultParent(uiFlexBox)
uiWindowsControlDefaultSetParent(uiFlexBox)
uiWindowsControlDefaultToplevel(uiFlexBox)
uiWindowsControlDefaultVisible(uiFlexBox)
uiWindowsControlDefaultShow(uiFlexBox)
uiWindowsControlDefaultHide(uiFlexBox)
uiWindowsControlDefaultEnabled(uiFlexBox)
uiWindowsControlDefaultEnable(uiFlexBox)
uiWindowsControlDefaultDisable(uiFlexBox)

static void uiFlexBoxSyncEnableState(uiWindowsControl *c, int enabled)
{
	uiFlexBox *f = uiFlexBox(c);

	if (uiWindowsShouldStopSyncEnableState(uiWindowsControl(f), enabled))
		return;
	for (const struct flexChild &fc : *(f->controls))
		uiWindowsControlSyncEnableState(uiWindowsControl(fc.c), enabled);
}

uiWindowsControlDefaultSetParentHWND(uiFlexBox)

static void uiFlexBoxMinimumSize(uiWindowsControl *c, int *width, int *height)
{
	uiFlexBox *f = uiFlexBox(c);
	int maxRight = 0;
	int maxBottom = 0;

	for (const struct flexChild &fc : *(f->controls)) {
		if (!uiControlVisible(fc.c))
			continue;
		int right = fc.x + fc.width;
		int bottom = fc.y + fc.height;
		if (right > maxRight)
			maxRight = right;
		if (bottom > maxBottom)
			maxBottom = bottom;
	}

	*width = maxRight;
	*height = maxBottom;
}

static void uiFlexBoxMinimumSizeChanged(uiWindowsControl *c)
{
	uiFlexBox *f = uiFlexBox(c);

	if (uiWindowsControlTooSmall(uiWindowsControl(f))) {
		uiWindowsControlContinueMinimumSizeChanged(uiWindowsControl(f));
		return;
	}
	flexboxRelayout(f);
}

uiWindowsControlDefaultLayoutRect(uiFlexBox)
uiWindowsControlDefaultAssignControlIDZOrder(uiFlexBox)

static void uiFlexBoxChildVisibilityChanged(uiWindowsControl *c)
{
	uiWindowsControlMinimumSizeChanged(c);
}

// Z-order estable: el último hijo está siempre arriba (como una pila)
static void flexboxArrangeChildren(uiFlexBox *f)
{
	if (f->controls->empty())
		return;

	// Usar HDWP para actualizar z-order de forma atómica (sin flicker)
	HDWP hdwp = BeginDeferWindowPos((int)f->controls->size());
	if (hdwp == NULL)
		return;

	// Iterar en reversa: el último elemento del vector queda en HWND_TOP
	HWND prev = HWND_TOP;

	for (int i = (int)f->controls->size() - 1; i >= 0; i--) {
		const struct flexChild &fc = (*(f->controls))[i];
		HWND hwnd = (HWND)uiControlHandle(fc.c);

		hdwp = DeferWindowPos(
			hdwp,
			hwnd,
			prev,
			0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW);

		if (hdwp == NULL)
			return;

		prev = hwnd;
	}

	EndDeferWindowPos(hdwp);
}

void uiFlexBoxAppend(uiFlexBox *f, uiControl *c, int x, int y, int width, int height)
{
	struct flexChild fc;

	fc.c = c;
	fc.x = x;
	fc.y = y;
	fc.width = width;
	fc.height = height;
	uiControlSetParent(fc.c, uiControl(f));
	uiWindowsControlSetParentHWND(uiWindowsControl(fc.c), f->hwnd);

	// Añadir WS_CLIPSIBLINGS al hijo para evitar que se dibuje sobre otros
	HWND childHwnd = (HWND)uiControlHandle(fc.c);
	LONG_PTR childStyle = GetWindowLongPtr(childHwnd, GWL_STYLE);
	childStyle |= WS_CLIPSIBLINGS;
	SetWindowLongPtr(childHwnd, GWL_STYLE, childStyle);

	f->controls->push_back(fc);
	flexboxArrangeChildren(f);
	uiWindowsControlMinimumSizeChanged(uiWindowsControl(f));
}

void uiFlexBoxSetChildPosition(uiFlexBox *f, int index, int x, int y, int width, int height)
{
	if (index < 0 || index >= (int)f->controls->size())
		return;
	
	struct flexChild &fc = (*(f->controls))[index];
	fc.x = x;
	fc.y = y;
	fc.width = width;
	fc.height = height;
	uiWindowsControlMinimumSizeChanged(uiWindowsControl(f));
}

void uiFlexBoxDelete(uiFlexBox *f, int index)
{
	if (index < 0 || index >= (int)f->controls->size())
		return;

	uiControl *c = (*(f->controls))[index].c;
	uiControlSetParent(c, NULL);
	uiWindowsControlSetParentHWND(uiWindowsControl(c), NULL);
	f->controls->erase(f->controls->begin() + index);
	flexboxArrangeChildren(f);
	uiWindowsControlMinimumSizeChanged(uiWindowsControl(f));
}

int uiFlexBoxNumChildren(uiFlexBox *f)
{
	return (int) f->controls->size();
}

static void onResize(uiWindowsControl *c)
{
	flexboxRelayout(uiFlexBox(c));
}

// Subclass para interceptar mensajes que podrían cambiar z-order
static LRESULT CALLBACK flexboxSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	uiFlexBox *f = (uiFlexBox *)dwRefData;

	switch (uMsg) {
	// Interceptar cuando un hijo intenta activarse
	case WM_PARENTNOTIFY:
		// Después de cualquier evento de mouse en un hijo, reafirmar z-order
		if (LOWORD(wParam) == WM_LBUTTONDOWN || 
		    LOWORD(wParam) == WM_MBUTTONDOWN || 
		    LOWORD(wParam) == WM_RBUTTONDOWN) {
			// Programar reafirmación del z-order después de que Windows procese el evento
			PostMessage(hwnd, WM_USER + 100, 0, 0);
		}
		break;
	case WM_USER + 100:
		// Reafirmar z-order
		flexboxArrangeChildren(f);
		return 0;
	case WM_NCDESTROY:
		RemoveWindowSubclass(hwnd, flexboxSubclassProc, uIdSubclass);
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

uiFlexBox *uiNewFlexBox(void)
{
	uiFlexBox *f;

	uiWindowsNewControl(uiFlexBox, f);

	f->hwnd = uiWindowsMakeContainer(uiWindowsControl(f), onResize);

	// EVITA FLICKER Y PELEAS DE Z-ORDER ENTRE HWNDs HIJOS
	LONG_PTR style = GetWindowLongPtr(f->hwnd, GWL_STYLE);
	style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	SetWindowLongPtr(f->hwnd, GWL_STYLE, style);

	f->controls = new std::vector<struct flexChild>;

	// Instalar subclass para mantener z-order estable
	SetWindowSubclass(f->hwnd, flexboxSubclassProc, 0, (DWORD_PTR)f);

	return f;
}
