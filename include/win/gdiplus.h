#ifndef WIN_GDIPLUS_H_
#define WIN_GDIPLUS_H_

#include <windows.h>
#include <stdbool.h>
#include <objidl.h>

typedef void* GdipBitmap;

typedef struct {
    unsigned char* pixels;  // RGBA format, caller must free()
    UINT width;
    UINT height;
} GdipPixelData;

bool gdipInit(void);
void gdipShutdown(void);
GdipBitmap gdipLoadFromStream(IStream* stream);
bool gdipGetPixels(GdipBitmap bitmap, GdipPixelData* outData);
void gdipFreeBitmap(GdipBitmap bitmap);

#endif // WIN_GDIPLUS_H_
