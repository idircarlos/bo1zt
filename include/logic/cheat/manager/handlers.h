#ifndef CHEAT_MANAGER_HANDLERS_H_
#define CHEAT_MANAGER_HANDLERS_H_

#include "logic/cheat/manager.h"

void cheatManagerHandleStateChange(CheatManager *manager);
void cheatManagerHandleGameAttach(CheatManager *manager);
void cheatManagerHandleGameDetach(CheatManager *manager);
void cheatManagerHandleGameStart(CheatManager *manager);

#endif // CHEAT_MANAGER_HANDLERS_H_
