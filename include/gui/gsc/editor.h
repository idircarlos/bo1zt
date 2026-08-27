#ifndef GUI_GSC_EDITOR_H_
#define GUI_GSC_EDITOR_H_

#include <stdbool.h>
#include <ui.h>

#include "client.h"

typedef enum {
    GSC_EDIT_UNDO,
    GSC_EDIT_REDO,
    GSC_EDIT_CUT,
    GSC_EDIT_COPY,
    GSC_EDIT_PASTE,
    GSC_EDIT_SELECT_ALL
} GscEditAction;

uiControl *gscEditorCreate(Client *client, uiWindow *window);
void gscEditorOpen(const char *path);
void gscEditorClose(const char *path);
void gscEditorFlush(void);
void gscEditorEdit(GscEditAction action);
void gscEditorSetWordWrap(bool enabled);
void gscEditorSetLineNumbers(bool enabled);

#endif // GUI_GSC_EDITOR_H_
