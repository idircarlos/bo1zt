#ifndef COMMAND_GRAPHICS_H_
#define COMMAND_GRAPHICS_H_

#include "logic/command.h"

typedef struct Controller Controller;

void commandGraphicsInit(Controller *controller);

bool commandFovHandle(Command command);
bool commandFovscaleHandle(Command command);
bool commandFpsHandle(Command command);
bool commandBorderlessHandle(Command command);
bool commandUnlimitfpsHandle(Command command);
bool commandDisablehudHandle(Command command);
bool commandDisablefogHandle(Command command);
bool commandFullbrightHandle(Command command);
bool commandColorizedHandle(Command command);

#endif // COMMAND_GRAPHICS_H_
