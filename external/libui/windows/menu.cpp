// 24 april 2015
#include "uipriv_windows.hpp"

struct uiMenuBar {
	std::vector<uiMenu *> *menus;
	BOOL finalized;
	BOOL hasQuit;
	BOOL hasPreferences;
	BOOL hasAbout;
};

struct uiMenu {
	uiMenuBar *menuBar;
	WCHAR *name;
	std::vector<uiMenuItem *> *items;
};

struct uiMenuItem {
	uiMenuBar *menuBar;
	WCHAR *name;
	WCHAR *shortcut;
	ACCEL accel;
	int type;
	WORD id;
	void (*onClicked)(uiMenuItem *, uiWindow *, void *);
	void *onClickedData;
	BOOL disabled;				// template for new instances; kept in sync with everything else
	BOOL checked;
	std::vector<HMENU> *hmenus;
	uiMenu *submenu;
};

enum {
	typeRegular,
	typeCheckbox,
	typeQuit,
	typePreferences,
	typeAbout,
	typeSeparator,
	typeSubmenu,
};

static std::vector<uiMenuBar *> menuBars;
static WORD curID = 100;			// start somewhere safe

static void sync(uiMenuItem *item)
{
	MENUITEMINFOW mi;

	ZeroMemory(&mi, sizeof (MENUITEMINFOW));
	mi.cbSize = sizeof (MENUITEMINFOW);
	mi.fMask = MIIM_STATE;
	if (item->disabled)
		mi.fState |= MFS_DISABLED;
	if (item->checked)
		mi.fState |= MFS_CHECKED;

	for (HMENU menu : *(item->hmenus))
		if (SetMenuItemInfo(menu, item->id, FALSE, &mi) == 0)
			logLastError(L"error synchronizing menu items");
}

static void defaultOnClicked(uiMenuItem *item, uiWindow *w, void *data)
{
	// do nothing
}

static void onQuitClicked(uiMenuItem *item, uiWindow *w, void *data)
{
	if (uiprivShouldQuit())
		uiQuit();
}

void uiMenuItemEnable(uiMenuItem *i)
{
	i->disabled = FALSE;
	sync(i);
}

void uiMenuItemDisable(uiMenuItem *i)
{
	i->disabled = TRUE;
	sync(i);
}

void uiMenuItemOnClicked(uiMenuItem *i, void (*f)(uiMenuItem *, uiWindow *, void *), void *data)
{
	if (i->type == typeQuit)
		uiprivUserBug("You can not call uiMenuItemOnClicked() on a Quit item; use uiOnShouldQuit() instead.");
	i->onClicked = f;
	i->onClickedData = data;
}

int uiMenuItemChecked(uiMenuItem *i)
{
	return i->checked != FALSE;
}

void uiMenuItemSetChecked(uiMenuItem *i, int checked)
{
	// use explicit values
	i->checked = FALSE;
	if (checked)
		i->checked = TRUE;
	sync(i);
}

typedef struct {
	int key;
	WORD virtualKey;
	const WCHAR *name;
} shortcutKey;

static const shortcutKey shortcutKeys[] = {
	{ uiExtKeyEscape,	VK_ESCAPE,	L"Esc" },
	{ uiExtKeyInsert,	VK_INSERT,	L"Ins" },
	{ uiExtKeyDelete,	VK_DELETE,	L"Del" },
	{ uiExtKeyHome,		VK_HOME,	L"Home" },
	{ uiExtKeyEnd,		VK_END,		L"End" },
	{ uiExtKeyPageUp,	VK_PRIOR,	L"PgUp" },
	{ uiExtKeyPageDown,	VK_NEXT,	L"PgDn" },
	{ uiExtKeyUp,		VK_UP,		L"Up" },
	{ uiExtKeyDown,		VK_DOWN,	L"Down" },
	{ uiExtKeyLeft,		VK_LEFT,	L"Left" },
	{ uiExtKeyRight,	VK_RIGHT,	L"Right" },
	{ uiExtKeyN0,		VK_NUMPAD0,	L"Num 0" },
	{ uiExtKeyN1,		VK_NUMPAD1,	L"Num 1" },
	{ uiExtKeyN2,		VK_NUMPAD2,	L"Num 2" },
	{ uiExtKeyN3,		VK_NUMPAD3,	L"Num 3" },
	{ uiExtKeyN4,		VK_NUMPAD4,	L"Num 4" },
	{ uiExtKeyN5,		VK_NUMPAD5,	L"Num 5" },
	{ uiExtKeyN6,		VK_NUMPAD6,	L"Num 6" },
	{ uiExtKeyN7,		VK_NUMPAD7,	L"Num 7" },
	{ uiExtKeyN8,		VK_NUMPAD8,	L"Num 8" },
	{ uiExtKeyN9,		VK_NUMPAD9,	L"Num 9" },
	{ uiExtKeyNDot,		VK_DECIMAL,	L"Num ." },
	{ uiExtKeyNEnter,	VK_RETURN,	L"Num Enter" },
	{ uiExtKeyNAdd,		VK_ADD,		L"Num +" },
	{ uiExtKeyNSubtract,	VK_SUBTRACT,	L"Num -" },
	{ uiExtKeyNMultiply,	VK_MULTIPLY,	L"Num *" },
	{ uiExtKeyNDivide,	VK_DIVIDE,	L"Num /" },
};

#define shortcutKeyCount (sizeof (shortcutKeys) / sizeof (*shortcutKeys))

static BOOL isFunctionKey(int key)
{
	return key >= uiExtKeyF1 && key <= uiExtKeyF12;
}

static BOOL isAlphanumericKey(int key)
{
	return (key >= '0' && key <= '9') ||
		(key >= 'A' && key <= 'Z') ||
		(key >= 'a' && key <= 'z');
}

static WCHAR upperKey(int key)
{
	if (key >= 'a' && key <= 'z')
		return (WCHAR) (key - 'a' + 'A');
	return (WCHAR) key;
}

static const shortcutKey *lookupShortcutKey(int key)
{
	for (size_t i = 0; i < shortcutKeyCount; i++)
		if (shortcutKeys[i].key == key)
			return &shortcutKeys[i];
	return NULL;
}

static BOOL isShortcutKey(int key)
{
	return isAlphanumericKey(key) || isFunctionKey(key) || lookupShortcutKey(key) != NULL;
}

static WORD shortcutVirtualKey(int key)
{
	if (isAlphanumericKey(key))
		return (WORD) upperKey(key);
	if (isFunctionKey(key))
		return (WORD) (VK_F1 + (key - uiExtKeyF1));
	return lookupShortcutKey(key)->virtualKey;
}

static WCHAR *shortcutKeyName(int key)
{
	if (isAlphanumericKey(key))
		return strf(L"%c", upperKey(key));
	if (isFunctionKey(key))
		return strf(L"F%d", key - uiExtKeyF1 + 1);
	return utf16dup(lookupShortcutKey(key)->name);
}

static WCHAR *shortcutName(int modifiers, int key)
{
	WCHAR *keyName;
	WCHAR *name;

	keyName = shortcutKeyName(key);
	name = strf(L"%s%s%s%s",
		(modifiers & uiModifierCtrl) != 0 ? L"Ctrl+" : L"",
		(modifiers & uiModifierAlt) != 0 ? L"Alt+" : L"",
		(modifiers & uiModifierShift) != 0 ? L"Shift+" : L"",
		keyName);
	uiprivFree(keyName);
	return name;
}

void uiMenuItemSetShortcut(uiMenuItem *i, int modifiers, int key)
{
	if (i->menuBar->finalized)
		uiprivUserBug("You can not set a menu item shortcut after the menu bar has been attached to a window.");
	if (i->type == typeSeparator)
		uiprivUserBug("You can not set a shortcut on a separator.");
	if (i->type == typeSubmenu)
		uiprivUserBug("You can not set a shortcut on a submenu.");
	if ((modifiers & uiModifierSuper) != 0)
		uiprivUserBug("Windows does not support the Super key in menu item shortcuts.");
	if (!isShortcutKey(key))
		uiprivUserBug("Unsupported menu item shortcut key %d.", key);

	if (i->shortcut != NULL)
		uiprivFree(i->shortcut);
	i->shortcut = shortcutName(modifiers, key);

	i->accel.fVirt = FVIRTKEY;
	if ((modifiers & uiModifierCtrl) != 0)
		i->accel.fVirt |= FCONTROL;
	if ((modifiers & uiModifierAlt) != 0)
		i->accel.fVirt |= FALT;
	if ((modifiers & uiModifierShift) != 0)
		i->accel.fVirt |= FSHIFT;
	i->accel.key = shortcutVirtualKey(key);
	i->accel.cmd = i->id;
}

static uiMenu *newMenu(uiMenuBar *mb, const char *name)
{
	uiMenu *m;

	if (mb->finalized)
		uiprivUserBug("You can not append a menu after the menu bar has been attached to a window.");

	m = uiprivNew(uiMenu);
	m->menuBar = mb;
	m->name = toUTF16(name);
	m->items = new std::vector<uiMenuItem *>;
	return m;
}

static uiMenuItem *newItem(uiMenu *m, int type, const char *name)
{
	uiMenuItem *item;

	if (m->menuBar->finalized)
		uiprivUserBug("You can not create a new menu item after the menu bar has been attached to a window.");

	item = uiprivNew(uiMenuItem);
	item->menuBar = m->menuBar;
	item->hmenus = new std::vector<HMENU>;
	m->items->push_back(item);

	item->type = type;
	switch (item->type) {
	case typeQuit:
		item->name = toUTF16("Quit");
		break;
	case typePreferences:
		item->name = toUTF16("Preferences...");
		break;
	case typeAbout:
		item->name = toUTF16("About");
		break;
	case typeSeparator:
		break;
	default:
		item->name = toUTF16(name);
		break;
	}

	if (item->type != typeSeparator) {
		item->id = curID;
		curID++;
	}

	if (item->type == typeQuit) {
		// can't call uiMenuItemOnClicked() here
		item->onClicked = onQuitClicked;
		item->onClickedData = NULL;
	} else
		uiMenuItemOnClicked(item, defaultOnClicked, NULL);

	return item;
}

uiMenuItem *uiMenuAppendItem(uiMenu *m, const char *name)
{
	return newItem(m, typeRegular, name);
}

uiMenuItem *uiMenuAppendCheckItem(uiMenu *m, const char *name)
{
	return newItem(m, typeCheckbox, name);
}

uiMenuItem *uiMenuAppendQuitItem(uiMenu *m)
{
	if (m->menuBar->hasQuit)
		uiprivUserBug("You can not have multiple Quit menu items in a menu bar.");
	m->menuBar->hasQuit = TRUE;
	newItem(m, typeSeparator, NULL);
	return newItem(m, typeQuit, NULL);
}

uiMenuItem *uiMenuAppendPreferencesItem(uiMenu *m)
{
	if (m->menuBar->hasPreferences)
		uiprivUserBug("You can not have multiple Preferences menu items in a menu bar.");
	m->menuBar->hasPreferences = TRUE;
	newItem(m, typeSeparator, NULL);
	return newItem(m, typePreferences, NULL);
}

uiMenuItem *uiMenuAppendAboutItem(uiMenu *m)
{
	if (m->menuBar->hasAbout)
		// TODO place these uiprivImplBug() and uiprivUserBug() strings in a header
		uiprivUserBug("You can not have multiple About menu items in a menu bar.");
	m->menuBar->hasAbout = TRUE;
	newItem(m, typeSeparator, NULL);
	return newItem(m, typeAbout, NULL);
}

void uiMenuAppendSeparator(uiMenu *m)
{
	newItem(m, typeSeparator, NULL);
}

uiMenu *uiMenuAppendSubmenu(uiMenu *m, const char *name)
{
	uiMenuItem *item;

	item = newItem(m, typeSubmenu, name);
	item->submenu = newMenu(m->menuBar, name);
	return item->submenu;
}

uiMenuBar *uiNewMenuBar(void)
{
	uiMenuBar *mb;

	mb = uiprivNew(uiMenuBar);
	mb->menus = new std::vector<uiMenu *>;
	menuBars.push_back(mb);
	return mb;
}

uiMenu *uiMenuBarAppendMenu(uiMenuBar *mb, const char *name)
{
	uiMenu *m;

	m = newMenu(mb, name);
	mb->menus->push_back(m);
	return m;
}

static HMENU makeMenu(uiMenu *m);

// AppendMenuW() spends the ID slot of a popup item on its HMENU, so the ID that sync() looks
// items up by has to be written back afterwards
static void setPopupItemID(HMENU menu, WORD id)
{
	MENUITEMINFOW mi;

	ZeroMemory(&mi, sizeof (MENUITEMINFOW));
	mi.cbSize = sizeof (MENUITEMINFOW);
	mi.fMask = MIIM_ID;
	mi.wID = id;
	if (SetMenuItemInfoW(menu, GetMenuItemCount(menu) - 1, TRUE, &mi) == 0)
		logLastError(L"error setting submenu item ID");
}

static void appendMenuItem(HMENU menu, uiMenuItem *item)
{
	UINT uFlags;
	WCHAR *text;
	UINT_PTR idNewItem;

	uFlags = MF_SEPARATOR;
	if (item->type != typeSeparator) {
		uFlags = MF_STRING;
		if (item->disabled)
			uFlags |= MF_DISABLED | MF_GRAYED;
		if (item->checked)
			uFlags |= MF_CHECKED;
	}
	text = item->name;
	if (item->shortcut != NULL)
		text = strf(L"%s\t%s", item->name, item->shortcut);
	idNewItem = item->id;
	if (item->type == typeSubmenu) {
		uFlags |= MF_POPUP;
		idNewItem = (UINT_PTR) makeMenu(item->submenu);
	}
	if (AppendMenuW(menu, uFlags, idNewItem, text) == 0)
		logLastError(L"error appending menu item");
	if (text != item->name)
		uiprivFree(text);
	if (item->type == typeSubmenu)
		setPopupItemID(menu, item->id);

	item->hmenus->push_back(menu);
}

static HMENU makeMenu(uiMenu *m)
{
	HMENU menu;

	menu = CreatePopupMenu();
	if (menu == NULL)
		logLastError(L"error creating menu");
	for (uiMenuItem *item : *(m->items))
		appendMenuItem(menu, item);
	return menu;
}

HMENU uiprivMenuBarMake(uiMenuBar *mb)
{
	HMENU menubar;
	HMENU menu;

	mb->finalized = TRUE;

	menubar = CreateMenu();
	if (menubar == NULL)
		logLastError(L"error creating menubar");

	for (uiMenu *m : *(mb->menus)) {
		menu = makeMenu(m);
		if (AppendMenuW(menubar, MF_POPUP | MF_STRING, (UINT_PTR) menu, m->name) == 0)
			logLastError(L"error appending menu to menubar");
	}

	return menubar;
}

static void menuCollectAccelerators(uiMenu *m, std::vector<ACCEL> *accels)
{
	for (uiMenuItem *item : *(m->items)) {
		if (item->type == typeSubmenu)
			menuCollectAccelerators(item->submenu, accels);
		else if (item->accel.key != 0)
			accels->push_back(item->accel);
	}
}

HACCEL uiprivMenuBarMakeAccelerators(uiMenuBar *mb)
{
	std::vector<ACCEL> accels;
	HACCEL table;

	for (uiMenu *m : *(mb->menus))
		menuCollectAccelerators(m, &accels);
	if (accels.empty())
		return NULL;

	table = CreateAcceleratorTableW(accels.data(), (int) accels.size());
	if (table == NULL)
		logLastError(L"error creating accelerator table");
	return table;
}

static BOOL menuRunEvent(uiMenu *m, WORD id, uiWindow *w)
{
	for (uiMenuItem *item : *(m->items)) {
		if (item->type == typeSubmenu) {
			if (menuRunEvent(item->submenu, id, w))
				return TRUE;
			continue;
		}
		if (item->id != id)
			continue;
		if (item->type == typeCheckbox)
			uiMenuItemSetChecked(item, !uiMenuItemChecked(item));
		(*(item->onClicked))(item, w, item->onClickedData);
		return TRUE;
	}
	return FALSE;
}

void uiprivMenuBarRunEvent(uiMenuBar *mb, WORD id, uiWindow *w)
{
	for (uiMenu *m : *(mb->menus))
		if (menuRunEvent(m, id, w))
			return;
}

static HMENU submenuAt(HMENU menu, size_t position)
{
	MENUITEMINFOW mi;

	ZeroMemory(&mi, sizeof (MENUITEMINFOW));
	mi.cbSize = sizeof (MENUITEMINFOW);
	mi.fMask = MIIM_SUBMENU;
	if (GetMenuItemInfoW(menu, position, TRUE, &mi) == 0) {
		logLastError(L"error getting menu to delete item references from");
		return NULL;
	}
	return mi.hSubMenu;
}

static void menuForgetHMENU(uiMenu *m, HMENU submenu)
{
	if (submenu == NULL)
		return;
	for (size_t i = 0; i < m->items->size(); i++) {
		uiMenuItem *item = (*(m->items))[i];

		for (size_t j = 0; j < item->hmenus->size(); j++)
			if ((*(item->hmenus))[j] == submenu) {
				item->hmenus->erase(item->hmenus->begin() + j);
				break;
			}
		if (item->type == typeSubmenu)
			menuForgetHMENU(item->submenu, submenuAt(submenu, i));
	}
}

void uiprivMenuBarForgetHMENU(uiMenuBar *mb, HMENU menubar)
{
	for (size_t i = 0; i < mb->menus->size(); i++)
		menuForgetHMENU((*(mb->menus))[i], submenuAt(menubar, i));
	// no need to worry about destroying any menus; destruction of the window they're in will do it for us
}

static void freeMenu(uiMenu *m)
{
	for (uiMenuItem *item : *(m->items)) {
		if (item->type == typeSubmenu)
			freeMenu(item->submenu);
		if (item->name != NULL)
			uiprivFree(item->name);
		if (item->shortcut != NULL)
			uiprivFree(item->shortcut);
		delete item->hmenus;
		uiprivFree(item);
	}
	delete m->items;
	uiprivFree(m->name);
	uiprivFree(m);
}

void uninitMenus(void)
{
	for (uiMenuBar *mb : menuBars) {
		for (uiMenu *m : *(mb->menus))
			freeMenu(m);
		delete mb->menus;
		uiprivFree(mb);
	}
	menuBars.clear();
	curID = 100;
}
