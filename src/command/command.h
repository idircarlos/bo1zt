#ifndef COMMAND_H_
#define COMMAND_H_

#include "../controller/controller.h"

typedef enum {
    COMMAND_NONE = -2,
    COMMAND_UNKNOWN = -1,
    COMMAND_NOCLIP,
    COMMAND_GOD,
    COMMAND_DEMIGOD,
    COMMAND_INVISIBLE,
    COMMAND_UFO,
    COMMAND_GIVE,
    COMMAND_FOV,
    COMMAND_FOVSCALE,
    COMMAND_FPS,
    COMMAND_NDOGS,
    COMMAND_INSTA,
    COMMAND_INFAMMO,
    COMMAND_TP,
    COMMAND_UWU,
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