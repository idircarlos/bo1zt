#ifndef COMMAND_H_
#define COMMAND_H_

#include "controller.h"

typedef enum {
    COMMAND_NONE = -2,
    COMMAND_UNKNOWN = -1,
    COMMAND_NOCLIP,
    COMMAND_GOD,
    COMMAND_INVISIBLE,
    COMMAND_GIVE,
    COMMAND_FOV,
    COMMAND_FOVSCALE,
    COMMAND_FPS,
    COMMAND_INSTA,
    COMMAND_INFAMMO,
    COMMAND_TP,
    COMMAND_PERK,
    COMMAND_UWU,
    COMMAND_NORECOIL,
    COMMAND_CROSSHAIR,
    COMMAND_SPEED,
    COMMAND_NOSHELLSHOCK,
    COMMAND_KNIFE,
    COMMAND_STATICBOX,
    COMMAND_THIRDPERSON,
    COMMAND_BORDERLESS,
    COMMAND_UNLIMITFPS,
    COMMAND_DISABLEHUD,
    COMMAND_DISABLEFOG,
    COMMAND_FULLBRIGHT,
    COMMAND_COLORIZED,
    COMMAND_DOGS,
    COMMAND_MONKEYS,
    COMMAND_THIEF,
} CommandName;

typedef struct Command {
    CommandName name;
    int argc;
    char **argv;
} Command;

void commandInit(Controller *controller);
Command commandBuild(const char *message);
bool commandHandle(Command command);

#endif // COMMAND_H_
