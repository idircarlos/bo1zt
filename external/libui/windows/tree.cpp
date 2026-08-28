// uiTree - Hierarchical list of items inserted imperatively, each one carrying user data
#include "uipriv_windows.hpp"

struct uiTreeItem {
	uiTree *t;
	HTREEITEM handle;
	void *data;
};

struct uiTree {
	uiWindowsControl c;
	HWND hwnd;
	HIMAGELIST icons;
	std::vector<uiTreeItem *> *items;
	void (*onSelectionChanged)(uiTree *, void *);
	void *onSelectionChangedData;
	void (*onItemActivated)(uiTree *, void *);
	void *onItemActivatedData;
	void (*onItemContextMenu)(uiTree *, uiTreeItem *, void *);
	void *onItemContextMenuData;
	uiTreeItem *rightClicked;
	BOOL inhibitSelectionChanged;
};

static uiTreeItem *treeItemFromHandle(uiTree *t, HTREEITEM handle)
{
	TVITEMW item;

	if (handle == NULL)
		return NULL;

	ZeroMemory(&item, sizeof (TVITEMW));
	item.mask = TVIF_PARAM;
	item.hItem = handle;
	if (SendMessageW(t->hwnd, TVM_GETITEMW, 0, (LPARAM) (&item)) == FALSE)
		return NULL;

	return (uiTreeItem *) (item.lParam);
}

static BOOL onWM_NOTIFY(uiControl *c, HWND hwnd, NMHDR *nmhdr, LRESULT *lResult)
{
	uiTree *t = uiTree(c);

	switch (nmhdr->code) {
	case TVN_SELCHANGEDW:
		if (t->inhibitSelectionChanged)
			return FALSE;
		(*(t->onSelectionChanged))(t, t->onSelectionChangedData);
		*lResult = 0;
		return TRUE;
	case NM_DBLCLK:
		(*(t->onItemActivated))(t, t->onItemActivatedData);
		*lResult = 0;
		return TRUE;
	case NM_RCLICK:
		if (t->rightClicked == NULL)
			return FALSE;
		(*(t->onItemContextMenu))(t, t->rightClicked, t->onItemContextMenuData);
		t->rightClicked = NULL;
		*lResult = 1;
		return TRUE;
	}
	return FALSE;
}

static uiTreeItem *treeItemAt(uiTree *t, int x, int y)
{
	TVHITTESTINFO hit;

	ZeroMemory(&hit, sizeof (TVHITTESTINFO));
	hit.pt.x = x;
	hit.pt.y = y;
	SendMessageW(t->hwnd, TVM_HITTEST, 0, (LPARAM) (&hit));
	if ((hit.flags & TVHT_ONITEM) == 0)
		return NULL;

	return treeItemFromHandle(t, hit.hItem);
}

// the treeview selects its first item when it gains the focus without a
// selection, so the right clicked item has to be selected before that happens
static void treeSelectRightClicked(uiTree *t, LPARAM pos)
{
	t->rightClicked = treeItemAt(t, GET_X_LPARAM(pos), GET_Y_LPARAM(pos));
	if (t->rightClicked == NULL)
		return;

	t->inhibitSelectionChanged = TRUE;
	SendMessageW(t->hwnd, TVM_SELECTITEM, (WPARAM) TVGN_CARET, (LPARAM) (t->rightClicked->handle));
	t->inhibitSelectionChanged = FALSE;
}

static LRESULT CALLBACK treeSubProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	uiTree *t = uiTree(dwRefData);

	switch (uMsg) {
	case WM_RBUTTONDOWN:
		treeSelectRightClicked(t, lParam);
		break;
	case WM_NCDESTROY:
		if (RemoveWindowSubclass(hwnd, treeSubProc, uIdSubclass) == FALSE)
			logLastError(L"error removing uiTree subclass");
		break;
	}
	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

static void treeFreeItems(uiTree *t)
{
	for (uiTreeItem *item : *(t->items))
		uiprivFree(item);
	t->items->clear();
}

static void uiTreeDestroy(uiControl *c)
{
	uiTree *t = uiTree(c);

	uiWindowsUnregisterWM_NOTIFYHandler(t->hwnd);
	uiWindowsEnsureDestroyWindow(t->hwnd);
	treeFreeItems(t);
	delete t->items;
	if (t->icons != NULL)
		ImageList_Destroy(t->icons);
	uiFreeControl(uiControl(t));
}

uiWindowsControlAllDefaultsExceptDestroy(uiTree)

#define treeMinWidth 107		/* in line with other controls */
#define treeMinHeight (14 * 5)	/* roughly five items */

static void uiTreeMinimumSize(uiWindowsControl *c, int *width, int *height)
{
	uiTree *t = uiTree(c);
	uiWindowsSizing sizing;
	int x, y;

	x = treeMinWidth;
	y = treeMinHeight;
	uiWindowsGetSizing(t->hwnd, &sizing);
	uiWindowsSizingDlgUnitsToPixels(&sizing, &x, &y);
	*width = x;
	*height = y;
}

static void defaultOnSelectionChanged(uiTree *t, void *data)
{
	// do nothing
}

static void defaultOnItemActivated(uiTree *t, void *data)
{
	// do nothing
}

static void defaultOnItemContextMenu(uiTree *t, uiTreeItem *item, void *data)
{
	// do nothing
}

void uiTreeOnSelectionChanged(uiTree *t, void (*f)(uiTree *t, void *data), void *data)
{
	t->onSelectionChanged = f;
	t->onSelectionChangedData = data;
}

void uiTreeOnItemActivated(uiTree *t, void (*f)(uiTree *t, void *data), void *data)
{
	t->onItemActivated = f;
	t->onItemActivatedData = data;
}

void uiTreeOnItemContextMenu(uiTree *t, void (*f)(uiTree *t, uiTreeItem *item, void *data), void *data)
{
	t->onItemContextMenu = f;
	t->onItemContextMenuData = data;
}

uiTreeItem *uiTreeAppend(uiTree *t, uiTreeItem *parent, const char *text)
{
	uiTreeItem *item;
	TVINSERTSTRUCTW insert;
	WCHAR *wtext;

	item = uiprivNew(uiTreeItem);
	item->t = t;
	wtext = toUTF16(text);

	ZeroMemory(&insert, sizeof (TVINSERTSTRUCTW));
	insert.hParent = parent != NULL ? parent->handle : TVI_ROOT;
	insert.hInsertAfter = TVI_LAST;
	insert.item.mask = TVIF_TEXT | TVIF_PARAM;
	insert.item.pszText = wtext;
	insert.item.lParam = (LPARAM) item;

	item->handle = (HTREEITEM) SendMessageW(t->hwnd, TVM_INSERTITEMW, 0, (LPARAM) (&insert));
	uiprivFree(wtext);

	if (item->handle == NULL) {
		logLastError(L"error calling TVM_INSERTITEMW in uiTreeAppend()");
		uiprivFree(item);
		return NULL;
	}

	t->items->push_back(item);
	return item;
}

void uiTreeClear(uiTree *t)
{
	if (SendMessageW(t->hwnd, TVM_DELETEITEM, 0, (LPARAM) TVI_ROOT) == FALSE)
		logLastError(L"error calling TVM_DELETEITEM in uiTreeClear()");
	treeFreeItems(t);
}

void uiTreeExpandAll(uiTree *t)
{
	for (uiTreeItem *item : *(t->items))
		SendMessageW(t->hwnd, TVM_EXPAND, (WPARAM) TVE_EXPAND, (LPARAM) (item->handle));
}

void uiTreeCollapseAll(uiTree *t)
{
	for (uiTreeItem *item : *(t->items))
		SendMessageW(t->hwnd, TVM_EXPAND, (WPARAM) TVE_COLLAPSE, (LPARAM) (item->handle));
}

uiTreeItem *uiTreeSelected(uiTree *t)
{
	HTREEITEM handle;

	handle = (HTREEITEM) SendMessageW(t->hwnd, TVM_GETNEXTITEM, (WPARAM) TVGN_CARET, 0);
	return treeItemFromHandle(t, handle);
}

static bool treeEnsureIcons(uiTree *t)
{
	if (t->icons != NULL)
		return true;

	t->icons = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
		ILC_COLOR32, 0, 4);
	if (t->icons == NULL) {
		logLastError(L"error creating the uiTree image list");
		return false;
	}
	SendMessageW(t->hwnd, TVM_SETIMAGELIST, (WPARAM) TVSIL_NORMAL, (LPARAM) (t->icons));
	return true;
}

int uiTreeAppendIcon(uiTree *t, uiImage *image)
{
	IWICBitmap *source;
	HBITMAP bitmap;
	HDC dc;
	int index;

	if (!treeEnsureIcons(t))
		return -1;

	dc = GetDC(t->hwnd);
	source = uiprivImageAppropriateForDC(image, dc);
	if (source == NULL || uiprivWICToGDI(source, dc,
		GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), &bitmap) != S_OK) {
		ReleaseDC(t->hwnd, dc);
		return -1;
	}
	ReleaseDC(t->hwnd, dc);

	index = ImageList_Add(t->icons, bitmap, NULL);
	DeleteObject(bitmap);
	if (index < 0)
		logLastError(L"error adding an icon to the uiTree image list");
	return index;
}

void uiTreeItemSetIcon(uiTreeItem *item, int icon)
{
	TVITEMW tv;

	ZeroMemory(&tv, sizeof (TVITEMW));
	tv.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tv.hItem = item->handle;
	tv.iImage = icon;
	tv.iSelectedImage = icon;
	if (SendMessageW(item->t->hwnd, TVM_SETITEMW, 0, (LPARAM) (&tv)) == FALSE)
		logLastError(L"error calling TVM_SETITEMW in uiTreeItemSetIcon()");
}

void uiTreeItemSetData(uiTreeItem *item, void *data)
{
	item->data = data;
}

void *uiTreeItemData(uiTreeItem *item)
{
	return item->data;
}

uiTree *uiNewTree(void)
{
	uiTree *t;

	uiWindowsNewControl(uiTree, t);

	t->icons = NULL;
	t->items = new std::vector<uiTreeItem *>;
	t->hwnd = uiWindowsEnsureCreateControlHWND(WS_EX_CLIENTEDGE,
		WC_TREEVIEWW, L"",
		TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS | WS_TABSTOP | WS_HSCROLL | WS_VSCROLL,
		hInstance, NULL,
		TRUE);

	// the chevrons of the Explorer theme instead of the legacy +/- boxes
	SetWindowTheme(t->hwnd, L"Explorer", NULL);

	if (SetWindowSubclass(t->hwnd, treeSubProc, 0, (DWORD_PTR) t) == FALSE)
		logLastError(L"error subclassing treeview to handle context menus");

	uiWindowsRegisterWM_NOTIFYHandler(t->hwnd, onWM_NOTIFY, uiControl(t));
	uiTreeOnSelectionChanged(t, defaultOnSelectionChanged, NULL);
	uiTreeOnItemActivated(t, defaultOnItemActivated, NULL);
	uiTreeOnItemContextMenu(t, defaultOnItemContextMenu, NULL);

	return t;
}
