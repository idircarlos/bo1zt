#ifndef EMBEDDED_FONTS_H_
#define EMBEDDED_FONTS_H_

#include <stdbool.h>

void fontsInit(void);
void fontsCleanup(void);
bool fontsLoad(int resourceId);

#endif // EMBEDDED_FONTS_H_
