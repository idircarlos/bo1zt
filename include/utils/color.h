#ifndef COLOR_H_
#define COLOR_H_

#include <stdint.h>
#include <stdbool.h>

#define RGBA_COLOR_WHITE rgbaColorCreate(255, 255, 255, 255)

typedef struct {
    uint8_t r, g, b;
} RGBColor;

typedef struct {
    uint8_t r, g, b, a;
} RGBAColor;

typedef uint32_t HEXColor;

typedef struct {
    double hue, saturation, value;
} HSVColor;

RGBAColor rgbaColorCreate(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

bool hexColorParse(const char *text, HEXColor *out);
RGBColor hexColorToRgbColor(HEXColor hex);

HSVColor rgbColorToHsvColor(RGBColor rgb);
bool rgbColorIsGrey(RGBColor rgb);

bool hsvColorIsGrey(HSVColor hsv);
double hsvColorDistance(HSVColor target, HSVColor candidate);
double hsvColorBrightnessDistance(HSVColor target, HSVColor candidate);

#endif // COLOR_H_
