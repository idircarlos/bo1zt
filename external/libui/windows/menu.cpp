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
	WCHAR *name;
	int type;
	WORD id;
	void (*onClicked)(uiMenuItem *, uiWindow *, void *);
	void *onClickedData;
	BOOL disabled;				// template for new instances; kept in sync with everything else
	BOOL checked;
	std::vector<HMENU> *hmenus;
};

enum {
	typeRegular,
	typeCheckbox,
	typeQuit,
	typePreferences,
	typeAbout,
	typeSeparator,
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

static uiMenuItem *newItem(uiMenu *m, int type, const char *name)
{
	uiMenuItem *item;

	if (m->menuBar->finalized)
		uiprivUserBug("You can not create a new menu item after the menu bar has been attached to a window.");

	item = uiprivNew(uiMenuItem);
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

	if (mb->finalized)
		uiprivUserBug("You can not append a menu after the menu bar has been attached to a window.");

	m = uiprivNew(uiMenu);
	m->menuBar = mb;
	m->name = toUTF16(name);
	m->items = new std::vector<uiMenuItem *>;
	mb->menus->push_back(m);
	return m;
}

static void appendMenuItem(HMENU menu, uiMenuItem *item)
{
	UINT uFlags;

	uFlags = MF_SEPARATOR;
	if (item->type != typeSeparator) {
		uFlags = MF_STRING;
		if (item->disabled)
			uFlags |= MF_DISABLED | MF_GRAYED;
		if (item->checked)
			uFlags |= MF_CHECKED;
	}
	if (AppendMenuW(menu, uFlags, item->id, item->name) == 0)
		logLastError(L"error appending menu item");

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

void uiprivMenuBarRunEvent(uiMenuBar *mb, WORD id, uiWindow *w)
{
	for (uiMenu *m : *(mb->menus))
		for (uiMenuItem *item : *(m->items)) {
			if (item->id != id)
				continue;
			if (item->type == typeCheckbox)
				uiMenuItemSetChecked(item, !uiMenuItemChecked(item));
			(*(item->onClicked))(item, w, item->onClickedData);
			return;
		}
}

static void menuForgetHMENU(uiMenu *m, HMENU submenu)
{
	for (uiMenuItem *item : *(m->items))
		for (size_t i = 0; i < item->hmenus->size(); i++)
			if ((*(item->hmenus))[i] == submenu) {
				item->hmenus->erase(item->hmenus->begin() + i);
				break;
			}
}

void uiprivMenuBarForgetHMENU(uiMenuBar *mb, HMENU menubar)
{
	MENUITEMINFOW mi;

	for (size_t i = 0; i < mb->menus->size(); i++) {
		ZeroMemory(&mi, sizeof (MENUITEMINFOW));
		mi.cbSize = sizeof (MENUITEMINFOW);
		mi.fMask = MIIM_SUBMENU;
		if (GetMenuItemInfoW(menubar, i, TRUE, &mi) == 0) {
			logLastError(L"error getting menu to delete item references from");
			continue;
		}
		menuForgetHMENU((*(mb->menus))[i], mi.hSubMenu);
	}
	// no need to worry about destroying any menus; destruction of the window they're in will do it for us
}

static void freeMenu(uiMenu *m)
{
	for (uiMenuItem *item : *(m->items)) {
		if (item->name != NULL)
			uiprivFree(item->name);
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
