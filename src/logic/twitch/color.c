#include "logic/twitch/color.h"

#include "utils/color.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    ChatColor color;
    RGBColor rgb;
} ChatColorEntry;

static const ChatColorEntry CHAT_COLOR_TABLE[] = {
    {CHAT_COLOR_BLACK,       {  0,   0,   0}},
    {CHAT_COLOR_RED,         {255,   0,   0}},
    {CHAT_COLOR_GREEN,       {  0, 255,   0}},
    {CHAT_COLOR_YELLOW,      {255, 255,   0}},
    {CHAT_COLOR_DARK_BLUE,   {  0,   0, 255}},
    {CHAT_COLOR_CYAN,        {  0, 255, 255}},
    {CHAT_COLOR_PINK,        {255,   0, 255}},
    {CHAT_COLOR_WHITE,       {255, 255, 255}},
    {CHAT_COLOR_GRAY,        {128, 128, 128}},
    {CHAT_COLOR_BROWN,       {150, 100,  50}},
    {CHAT_COLOR_DARK_ORANGE, {200, 130,  60}},
    {CHAT_COLOR_BLUE,        {100, 180, 255}},
    {CHAT_COLOR_DARK_GREEN,  {  0, 128,   0}},
    {CHAT_COLOR_DARK_RED,    {128,   0,   0}},
    {CHAT_COLOR_LIGHT_GRAY,  {192, 192, 192}},
    {CHAT_COLOR_ORANGE,      {255, 140,   0}},
    {CHAT_COLOR_PURPLE,      {150,  60, 220}},
};

#define CHAT_COLOR_COUNT (sizeof(CHAT_COLOR_TABLE) / sizeof(CHAT_COLOR_TABLE[0]))

// Closest game color to a Twitch hex, matched in HSV because RGB proximity is not visual
// proximity. Greys have no real hue, so they form a separate pool matched by brightness alone.
ChatColor twitchColorToChatColor(const char *hex) {
    HEXColor parsed;
    if (!hexColorParse(hex, &parsed)) return CHAT_COLOR_WHITE;

    HSVColor target = rgbColorToHsvColor(hexColorToRgbColor(parsed));
    bool wantGrey = hsvColorIsGrey(target);

    size_t closest = 0;
    double best = -1.0;
    for (size_t i = 0; i < CHAT_COLOR_COUNT; i++) {
        if (rgbColorIsGrey(CHAT_COLOR_TABLE[i].rgb) != wantGrey) continue;
        HSVColor candidate = rgbColorToHsvColor(CHAT_COLOR_TABLE[i].rgb);
        double distance = wantGrey ? hsvColorBrightnessDistance(target, candidate)
                                   : hsvColorDistance(target, candidate);
        if (best < 0.0 || distance < best) {
            closest = i;
            best = distance;
        }
    }
    return CHAT_COLOR_TABLE[closest].color;
}
