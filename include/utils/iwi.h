#ifndef UTILS_IWI_H_
#define UTILS_IWI_H_

#include <stdbool.h>

typedef struct {
    unsigned char *pixels;
    int width;
    int height;
} IwiImage;

bool iwiLoad(const char *path, IwiImage *out, char *errMsg, int errSize);
void iwiFree(IwiImage *img);

#endif // UTILS_IWI_H_
