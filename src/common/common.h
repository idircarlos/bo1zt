#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>
#include <stdbool.h>

#define COLOR_WHITE colorCreate(255, 255, 255, 255)

typedef struct {
    uint8_t r, g, b, a;
} Color;

Color colorCreate(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

typedef struct {
    uint32_t x, y, w, h;
} Rect;

Rect rectCreate(uint32_t x, uint32_t y, uint32_t w, uint32_t h);

typedef uint32_t Flags;

void flagsAdd(Flags *flags, Flags mask);
void flagsRemove(Flags *flags, Flags mask);
bool flagsContains(Flags flags, Flags mask);
void flagsClear(Flags *flags);

typedef enum {
    CT_INT,
    CT_FLOAT,
    CT_STRING,
    CT_BOOL,
} CType;

#endif // COMMON_H_