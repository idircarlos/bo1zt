#ifndef CHEAT_MANAGER_H_
#define CHEAT_MANAGER_H_

#include <stdbool.h>

// Forward declarations
typedef struct Controller Controller;
typedef struct CheatManager CheatManager;

// Condition flags for cheat application
typedef enum {
    CHEAT_COND_NONE = 0,
    CHEAT_COND_NO_TIM = 1 << 0,           // TIM must not be running
    CHEAT_COND_GAME_ONGOING = 1 << 1,     // Zombies game must be ongoing
    CHEAT_COND_GAME_READY = 1 << 2,       // Black Ops game must be ready
} CheatCondition;

// Result of cheat operation
typedef enum {
    CHEAT_RESULT_OK,              // Cheat was successfully applied
    CHEAT_RESULT_CONDITION_NOT_MET, // Conditions not met, Config updated but not applied
    CHEAT_RESULT_API_FAILED,      // API call failed
    CHEAT_RESULT_NO_CHANGE,       // Value unchanged, no action taken
} CheatResult;

// Lifecycle
CheatManager *cheatManagerCreate(Controller *controller);
void cheatManagerDestroy(CheatManager *manager);

// Persistence
void cheatManagerSave(CheatManager *manager);

#endif // CHEAT_MANAGER_H_
