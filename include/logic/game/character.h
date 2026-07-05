#ifndef CHARACTER_H_
#define CHARACTER_H_

#include <stdbool.h>

typedef enum {
    CHARACTER_INVALID = -1,
    CHARACTER_DEMPSEY,
    CHARACTER_NIKOLAI,
    CHARACTER_TAKEO,
    CHARACTER_RICHTOFEN,
    CHARACTER_RANDOM,
} Character;

const char *characterName(Character character);
Character characterFromName(const char *name);

#endif // CHARACTER_H_