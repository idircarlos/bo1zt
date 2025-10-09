#include <stdio.h>
#include "gui/gui.h"
#include "memory/memory.h"
#include "controller/controller.h"

int main(void) {
    Controller *controller = controllerCreate("BlackOps.exe");
    if (!controller) {
        fprintf(stderr, "Could not create controller. Make sure the game is running.\n");
        return 1;
    }
    guiInit(controller);

    
    guiRun();
    guiCleanup();
    return 0;
}