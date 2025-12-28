#ifndef COMMAND_H_
#define COMMAND_H_

#include <stdbool.h>

typedef struct CommandManager CommandManager;

typedef enum {
    COMMAND_NONE,
    COMMAND_UNKNOWN,
    // Hacks
    COMMAND_GOD,
    COMMAND_NOCLIP,
    COMMAND_INVISIBLE,
    COMMAND_INFAMMO,
    COMMAND_INSTA,
    COMMAND_NORECOIL,
    COMMAND_NOSHELLSHOCK,
    COMMAND_SPEED,
    COMMAND_KNIFE,
    COMMAND_CROSSHAIR,
    COMMAND_STATICBOX,
    COMMAND_THIRDPERSON,
    // Graphics
    COMMAND_FOV,
    COMMAND_FOVSCALE,
    COMMAND_FPS,
    COMMAND_UNLIMITFPS,
    COMMAND_BORDERLESS,
    COMMAND_DISABLEHUD,
    COMMAND_DISABLEFOG,
    COMMAND_FULLBRIGHT,
    COMMAND_COLORIZED,
    // Misc
    COMMAND_GIVE,
    COMMAND_TP,
    COMMAND_PERK,
    COMMAND_RESTART,
    COMMAND_UWU,
    // Special rounds
    COMMAND_DOGS,
    COMMAND_MONKEYS,
    COMMAND_THIEF,
    // Info
    COMMAND_CLAYMORES,
    COMMAND_ENTITIES,
    COMMAND_SPH,
    COMMAND_TRADE,
    COMMAND_REVIVES,
} CommandName;

typedef struct Command {
    CommandName name;
    int argc;
    char **argv;
} Command;

Command commandBuild(CommandManager *manager, const char *message);
bool commandHandle(Command command);
Command *commandCopy(const Command *command);
void commandFree(Command *command);
const char *commandToString(const Command *command);

#endif // COMMAND_H_
