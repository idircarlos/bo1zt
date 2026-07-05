#ifndef SERVICE_WIDGETS_H_
#define SERVICE_WIDGETS_H_

#include "service.h"
#include "logic/config.h"

int serviceWidgetCount(void);
const char *serviceWidgetNameAt(int index);
const char *serviceWidgetLabelAt(int index);
int serviceWidgetIndexOf(const char *name);

ServiceResult serviceWidgetGet(Service *service, int index, WidgetConfig *configOut);
ServiceResult serviceWidgetReset(Service *service, int index);

typedef struct {
    bool hasEnabled;         bool enabled;
    bool hasFont;            const char *font;
    bool hasFontSize;        int fontSize;
    bool hasTextColor;       Color textColor;
    bool hasHideOutsideGame; bool hideOutsideGame;
    bool hasRect;            Rect rect;
} WidgetPatch;

ServiceResult serviceWidgetUpdate(Service *service, int index, const WidgetPatch *patch);

#endif // SERVICE_WIDGETS_H_
