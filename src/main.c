#include <stdio.h>
#include "gui/gui.h"
#include "memory/memory.h"

int main(void) {
    guiInit();

    // Memory testing. TODO: Remove this later.
    ProcessHandle *ph = memoryOpenProcess("BlackOps.exe");
    if (ph == NULL) {
        fprintf(stderr, "Could not find or open process chrome.exe. Make sure the game is running.\n");
        return 1;
    }

    int value;
    memoryRead(ph, 0x01C0A6C8, &value, sizeof(value));
    printf("Value at address 0x01C0A6C8 is %d\n", value);

    value = 9999;

    memoryWrite(ph, 0x01C0A6C8, &value, sizeof(value));
    printf("Writing %d at address 0x01C0A6C8\n", value);

    memoryCloseProcess(ph);
    
    guiRun();
    guiCleanup();
    return 0;
}