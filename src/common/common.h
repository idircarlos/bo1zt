#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>

typedef struct {
    uint8_t r, g, b;
} Color;

Color colorCreate(uint8_t r, uint8_t g, uint8_t b);

#endif // COMMON_H_