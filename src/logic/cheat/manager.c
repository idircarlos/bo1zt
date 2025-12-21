#include "logic/cheat/manager.h"
#include "logic/cheat/manager/manager_internal.h"
#include "logic/cheat/manager/state.h"
#include "controller.h"
#include "logic/config.h"
#include <stdlib.h>

CheatManager *cheatManagerCreate(Controller *controller) {
    if (!controller) return NULL;
    
    CheatManager *manager = (CheatManager *)malloc(sizeof(CheatManager));
    if (!manager) return NULL;
    
    manager->controller = controller;
    manager->config = controllerGetConfig(controller);
    appliedStateClear(&manager->applied);
    
    return manager;
}

void cheatManagerDestroy(CheatManager *manager) {
    if (manager) {
        free(manager);
    }
}

void cheatManagerSave(CheatManager *manager) {
    if (!manager || !manager->config) return;
    configSave(manager->config);
}
