#include "utils/color.h"

#include <math.h>

#define GREY_SATURATION 0.15
#define SHADE_WEIGHT 0.10

RGBAColor rgbaColorCreate(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    RGBAColor color = {r, g, b, a};
    return color;
}

static int hexDigit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool hexColorParse(const char *text, HEXColor *out) {
    if (!text) return false;
    if (*text == '#') text++;
    HEXColor value = 0;
    for (int i = 0; i < 6; i++) {
        int digit = hexDigit(text[i]);
        if (digit < 0) return false;
        value = value * 16 + (HEXColor)digit;
    }
    if (text[6] != '\0') return false;
    *out = value;
    return true;
}

RGBColor hexColorToRgbColor(HEXColor hex) {
    RGBColor rgb = {(uint8_t)(hex >> 16), (uint8_t)(hex >> 8), (uint8_t)hex};
    return rgb;
}

// Hue comes out of the sextant the max channel falls in; +6.0 keeps the red wrap positive.
HSVColor rgbColorToHsvColor(RGBColor rgb) {
    double r = rgb.r / 255.0, g = rgb.g / 255.0, b = rgb.b / 255.0;
    double max = fmax(r, fmax(g, b));
    double span = max - fmin(r, fmin(g, b));

    HSVColor hsv = {0.0, max > 0.0 ? span / max : 0.0, max};
    if (span > 0.0) {
        if (max == r) hsv.hue = fmod((g - b) / span + 6.0, 6.0);
        else if (max == g) hsv.hue = (b - r) / span + 2.0;
        else hsv.hue = (r - g) / span + 4.0;
        hsv.hue *= 60.0;
    }
    return hsv;
}

bool rgbColorIsGrey(RGBColor rgb) {
    return rgb.r == rgb.g && rgb.g == rgb.b;
}

bool hsvColorIsGrey(HSVColor hsv) {
    return hsv.saturation < GREY_SATURATION;
}

// Hue is a circle: the real gap is never more than 180 degrees. Result is normalized to 0..1.
static double hueGap(double left, double right) {
    double gap = fabs(left - right);
    return (gap > 180.0 ? 360.0 - gap : gap) / 180.0;
}

// Hue carries the identity of a color, so saturation and value only break ties.
double hsvColorDistance(HSVColor target, HSVColor candidate) {
    double dh = hueGap(target.hue, candidate.hue);
    double ds = target.saturation - candidate.saturation;
    double dv = target.value - candidate.value;
    return dh * dh + SHADE_WEIGHT * (ds * ds + dv * dv);
}

double hsvColorBrightnessDistance(HSVColor target, HSVColor candidate) {
    return fabs(target.value - candidate.value);
}
