#include "logic/game/character.h"

#include <string.h>

// Single source of truth for the character catalog: the kebab-case API name of
// each Character. Used by service/game (the /game/config resource), gui/character
// and the CLI. GameConfig.character stores the enum value.
static const struct {
    const char *name;
    Character value;
} CHARACTER_TABLE[] = {
    { "dempsey",   CHARACTER_DEMPSEY },
    { "nikolai",   CHARACTER_NIKOLAI },
    { "takeo",     CHARACTER_TAKEO },
    { "richtofen", CHARACTER_RICHTOFEN },
    { "random",    CHARACTER_RANDOM },
};

static const int CHARACTER_TABLE_SIZE =
    (int)(sizeof(CHARACTER_TABLE) / sizeof(CHARACTER_TABLE[0]));

const char *characterName(Character character) {
    for (int i = 0; i < CHARACTER_TABLE_SIZE; i++) {
        if (CHARACTER_TABLE[i].value == character) return CHARACTER_TABLE[i].name;
    }
    return "random";
}

Character characterFromName(const char *name) {
    if (name) {
        for (int i = 0; i < CHARACTER_TABLE_SIZE; i++) {
            if (strcmp(CHARACTER_TABLE[i].name, name) == 0) return CHARACTER_TABLE[i].value;
        }
    }
    return CHARACTER_INVALID;
}
