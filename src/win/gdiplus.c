#include "win/gdiplus.h"
#include <stdlib.h>
#include <string.h>

// GDI+ Flat API declarations
typedef struct {
    UINT32 GdiplusVersion;
    void* DebugEventCallback;
    BOOL SuppressBackgroundThread;
    BOOL SuppressExternalCodecs;
} GdiplusStartupInput;

typedef void* GpBitmap;
typedef void* GpImage;
typedef int GpStatus;

typedef struct {
    INT X, Y, Width, Height;
} GpRect;

typedef struct {
    UINT Width;
    UINT Height;
    INT Stride;
    int PixelFormat;
    void* Scan0;
    UINT_PTR Reserved;
} BitmapData;

#define PixelFormat32bppARGB 0x26200A
#define ImageLockModeRead 1

// GDI+ imports
#ifdef __cplusplus
extern "C" {
#endif
__declspec(dllimport) GpStatus __stdcall GdiplusStartup(ULONG_PTR*, const GdiplusStartupInput*, void*);
__declspec(dllimport) void __stdcall GdiplusShutdown(ULONG_PTR);
__declspec(dllimport) GpStatus __stdcall GdipCreateBitmapFromStream(IStream*, GpBitmap**);
__declspec(dllimport) GpStatus __stdcall GdipDisposeImage(GpImage*);
__declspec(dllimport) GpStatus __stdcall GdipGetImageWidth(GpImage*, UINT*);
__declspec(dllimport) GpStatus __stdcall GdipGetImageHeight(GpImage*, UINT*);
__declspec(dllimport) GpStatus __stdcall GdipBitmapLockBits(GpBitmap*, GpRect*, UINT, int, BitmapData*);
__declspec(dllimport) GpStatus __stdcall GdipBitmapUnlockBits(GpBitmap*, BitmapData*);
#ifdef __cplusplus
}
#endif

static ULONG_PTR gdipToken = 0;

bool gdipInit(void) {
    if (gdipToken) return true;
    GdiplusStartupInput input = {1, NULL, FALSE, FALSE};
    return GdiplusStartup(&gdipToken, &input, NULL) == 0;
}

void gdipShutdown(void) {
    if (gdipToken) {
        GdiplusShutdown(gdipToken);
        gdipToken = 0;
    }
}

GdipBitmap gdipLoadFromStream(IStream* stream) {
    if (!stream || !gdipInit()) return NULL;

    GpBitmap* bitmap = NULL;
    if (GdipCreateBitmapFromStream(stream, &bitmap) != 0) {
        return NULL;
    }
    return bitmap;
}

bool gdipGetPixels(GdipBitmap bitmap, GdipPixelData* outData) {
    if (!bitmap || !outData) return false;

    GpBitmap* bmp = (GpBitmap*)bitmap;

    UINT width, height;
    GdipGetImageWidth((GpImage*)bmp, &width);
    GdipGetImageHeight((GpImage*)bmp, &height);

    GpRect rect = {0, 0, (INT)width, (INT)height};
    BitmapData bmpData;
    memset(&bmpData, 0, sizeof(bmpData));

    if (GdipBitmapLockBits(bmp, &rect, ImageLockModeRead, PixelFormat32bppARGB, &bmpData) != 0) {
        return false;
    }

    // Allocate RGBA buffer
    unsigned char* pixels = (unsigned char*)malloc(width * height * 4);
    if (!pixels) {
        GdipBitmapUnlockBits(bmp, &bmpData);
        return false;
    }

    // Convert BGRA to RGBA and flip vertically for OpenGL
    unsigned char* src = (unsigned char*)bmpData.Scan0;
    for (UINT y = 0; y < height; y++) {
        for (UINT x = 0; x < width; x++) {
            UINT srcIdx = y * bmpData.Stride + x * 4;
            UINT dstIdx = ((height - 1 - y) * width + x) * 4;
            pixels[dstIdx + 0] = src[srcIdx + 2]; // R
            pixels[dstIdx + 1] = src[srcIdx + 1]; // G
            pixels[dstIdx + 2] = src[srcIdx + 0]; // B
            pixels[dstIdx + 3] = src[srcIdx + 3]; // A
        }
    }

    GdipBitmapUnlockBits(bmp, &bmpData);

    outData->pixels = pixels;
    outData->width = width;
    outData->height = height;
    return true;
}

void gdipFreeBitmap(GdipBitmap bitmap) {
    if (bitmap) {
        GdipDisposeImage((GpImage*)bitmap);
    }
}
