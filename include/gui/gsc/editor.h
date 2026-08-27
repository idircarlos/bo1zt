#ifndef GUI_GSC_EDITOR_H_
#define GUI_GSC_EDITOR_H_

#include <ui.h>

#include "client.h"

uiControl *gscEditorCreate(Client *client, uiWindow *window);
void gscEditorOpen(const char *path);
void gscEditorClose(const char *path);
void gscEditorFlush(void);

#endif // GUI_GSC_EDITOR_H_
