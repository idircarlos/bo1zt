#ifndef SERVICE_GRAPHICS_H_
#define SERVICE_GRAPHICS_H_

#include "service.h"
#include "logic/config.h"

typedef struct {
    bool hasFov;        int  fov;
    bool hasFovScale;   int  fovScale;
    bool hasFpsCap;     int  fpsCap;
    bool hasBorderless; bool borderless;
    bool hasUnlimitFps; bool unlimitFps;
    bool hasDisableHud; bool disableHud;
    bool hasDisableFog; bool disableFog;
    bool hasFullbright; bool fullbright;
    bool hasColorized;  bool colorized;
} GraphicsPatch;

ServiceResult serviceGraphicsGet(Service *service, GraphicsConfig *configOut);
ServiceResult serviceGraphicsReset(Service *service);
ServiceResult serviceGraphicsUpdate(Service *service, const GraphicsPatch *patch);

#endif // SERVICE_GRAPHICS_H_
