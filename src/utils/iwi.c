#include "utils/iwi.h"
#include "win/file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define IWI_VERSION_BO1  0x0D

#define IWI_FORMAT_ARGB32 0x01
#define IWI_FORMAT_RGB24  0x02
#define IWI_FORMAT_DXT1   0x0B
#define IWI_FORMAT_DXT3   0x0C
#define IWI_FORMAT_DXT5   0x0D

static void setErr(char *errMsg, int errSize, const char *msg) {
    if (errMsg && errSize > 0) {
        strncpy(errMsg, msg, (size_t)errSize - 1);
        errMsg[errSize - 1] = '\0';
    }
}

static uint16_t readU16(const unsigned char *p) {
    return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t readU32(const unsigned char *p) {
    return (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24));
}

static void expand565(uint16_t c, unsigned char *rgb) {
    unsigned r = (c >> 11) & 0x1F;
    unsigned g = (c >> 5) & 0x3F;
    unsigned b = c & 0x1F;
    rgb[0] = (unsigned char)((r << 3) | (r >> 2));
    rgb[1] = (unsigned char)((g << 2) | (g >> 4));
    rgb[2] = (unsigned char)((b << 3) | (b >> 2));
}

static void decodeColorBlock(const unsigned char *block, bool hasSeparateAlpha,
                             unsigned char palette[4][4]) {
    uint16_t c0 = readU16(block);
    uint16_t c1 = readU16(block + 2);

    unsigned char rgb0[3], rgb1[3];
    expand565(c0, rgb0);
    expand565(c1, rgb1);

    palette[0][0] = rgb0[0]; palette[0][1] = rgb0[1]; palette[0][2] = rgb0[2]; palette[0][3] = 255;
    palette[1][0] = rgb1[0]; palette[1][1] = rgb1[1]; palette[1][2] = rgb1[2]; palette[1][3] = 255;

    if (c0 > c1 || hasSeparateAlpha) {
        for (int i = 0; i < 3; i++) {
            palette[2][i] = (unsigned char)((2 * rgb0[i] + rgb1[i]) / 3);
            palette[3][i] = (unsigned char)((rgb0[i] + 2 * rgb1[i]) / 3);
        }
        palette[2][3] = 255;
        palette[3][3] = 255;
    } else {
        for (int i = 0; i < 3; i++) {
            palette[2][i] = (unsigned char)((rgb0[i] + rgb1[i]) / 2);
            palette[3][i] = 0;
        }
        palette[2][3] = 255;
        palette[3][3] = 0;
    }
}

static void writeColorBlock(unsigned char *out, int width, int height,
                            int bx, int by, const unsigned char palette[4][4],
                            const unsigned char *indexBytes) {
    uint32_t indices = readU32(indexBytes);
    for (int py = 0; py < 4; py++) {
        for (int px = 0; px < 4; px++) {
            int x = bx + px;
            int y = by + py;
            unsigned idx = (indices >> (2 * (py * 4 + px))) & 0x3;
            if (x >= width || y >= height) continue;
            unsigned char *dst = out + ((size_t)y * width + x) * 4;
            dst[0] = palette[idx][0];
            dst[1] = palette[idx][1];
            dst[2] = palette[idx][2];
            dst[3] = palette[idx][3];
        }
    }
}

static bool decodeDXT1(const unsigned char *data, size_t dataLen,
                       int width, int height, unsigned char *out) {
    int blocksX = (width + 3) / 4;
    int blocksY = (height + 3) / 4;
    size_t need = (size_t)blocksX * blocksY * 8;
    if (dataLen < need) return false;

    const unsigned char *p = data;
    for (int by = 0; by < blocksY; by++) {
        for (int bx = 0; bx < blocksX; bx++) {
            unsigned char palette[4][4];
            decodeColorBlock(p, false, palette);
            writeColorBlock(out, width, height, bx * 4, by * 4, palette, p + 4);
            p += 8;
        }
    }
    return true;
}

static bool decodeDXT3(const unsigned char *data, size_t dataLen,
                       int width, int height, unsigned char *out) {
    int blocksX = (width + 3) / 4;
    int blocksY = (height + 3) / 4;
    size_t need = (size_t)blocksX * blocksY * 16;
    if (dataLen < need) return false;

    const unsigned char *p = data;
    for (int by = 0; by < blocksY; by++) {
        for (int bx = 0; bx < blocksX; bx++) {
            const unsigned char *alphaBlock = p;
            const unsigned char *colorBlock = p + 8;

            unsigned char palette[4][4];
            decodeColorBlock(colorBlock, true, palette);
            writeColorBlock(out, width, height, bx * 4, by * 4, palette, colorBlock + 4);

            for (int py = 0; py < 4; py++) {
                uint16_t row = readU16(alphaBlock + py * 2);
                for (int px = 0; px < 4; px++) {
                    int x = bx * 4 + px;
                    int y = by * 4 + py;
                    if (x >= width || y >= height) continue;
                    unsigned a4 = (row >> (4 * px)) & 0xF;
                    out[((size_t)y * width + x) * 4 + 3] = (unsigned char)((a4 << 4) | a4);
                }
            }
            p += 16;
        }
    }
    return true;
}

static bool decodeDXT5(const unsigned char *data, size_t dataLen,
                       int width, int height, unsigned char *out) {
    int blocksX = (width + 3) / 4;
    int blocksY = (height + 3) / 4;
    size_t need = (size_t)blocksX * blocksY * 16;
    if (dataLen < need) return false;

    const unsigned char *p = data;
    for (int by = 0; by < blocksY; by++) {
        for (int bx = 0; bx < blocksX; bx++) {
            const unsigned char *alphaBlock = p;
            const unsigned char *colorBlock = p + 8;

            unsigned char palette[4][4];
            decodeColorBlock(colorBlock, true, palette);
            writeColorBlock(out, width, height, bx * 4, by * 4, palette, colorBlock + 4);

            unsigned a0 = alphaBlock[0];
            unsigned a1 = alphaBlock[1];
            unsigned char alpha[8];
            alpha[0] = (unsigned char)a0;
            alpha[1] = (unsigned char)a1;
            if (a0 > a1) {
                for (int i = 1; i < 7; i++)
                    alpha[i + 1] = (unsigned char)(((7 - i) * a0 + i * a1) / 7);
            } else {
                for (int i = 1; i < 5; i++)
                    alpha[i + 1] = (unsigned char)(((5 - i) * a0 + i * a1) / 5);
                alpha[6] = 0;
                alpha[7] = 255;
            }

            uint64_t bits = 0;
            for (int i = 0; i < 6; i++)
                bits |= (uint64_t)alphaBlock[2 + i] << (8 * i);

            for (int py = 0; py < 4; py++) {
                for (int px = 0; px < 4; px++) {
                    int x = bx * 4 + px;
                    int y = by * 4 + py;
                    unsigned idx = (unsigned)((bits >> (3 * (py * 4 + px))) & 0x7);
                    if (x >= width || y >= height) continue;
                    out[((size_t)y * width + x) * 4 + 3] = alpha[idx];
                }
            }
            p += 16;
        }
    }
    return true;
}

static bool decodeUncompressed(const unsigned char *data, size_t dataLen,
                               int width, int height, int format,
                               unsigned char *out) {
    size_t pixels = (size_t)width * height;
    if (format == IWI_FORMAT_ARGB32) {
        if (dataLen < pixels * 4) return false;
        for (size_t i = 0; i < pixels; i++) {
            out[i * 4 + 0] = data[i * 4 + 2];
            out[i * 4 + 1] = data[i * 4 + 1];
            out[i * 4 + 2] = data[i * 4 + 0];
            out[i * 4 + 3] = data[i * 4 + 3];
        }
        return true;
    }
    if (format == IWI_FORMAT_RGB24) {
        if (dataLen < pixels * 3) return false;
        for (size_t i = 0; i < pixels; i++) {
            out[i * 4 + 0] = data[i * 3 + 2];
            out[i * 4 + 1] = data[i * 3 + 1];
            out[i * 4 + 2] = data[i * 3 + 0];
            out[i * 4 + 3] = 255;
        }
        return true;
    }
    return false;
}

bool iwiLoad(const char *path, IwiImage *out, char *errMsg, int errSize) {
    if (!path || !out) {
        setErr(errMsg, errSize, "Invalid arguments");
        return false;
    }
    memset(out, 0, sizeof(*out));

    size_t size = 0;
    unsigned char *buf = (unsigned char *)fileReadAll(path, &size);
    if (!buf) {
        setErr(errMsg, errSize, "Could not open file");
        return false;
    }

    long fileSize = (long)size;
    if (fileSize < 0x10) {
        setErr(errMsg, errSize, "File too small to be a valid IWI");
        free(buf);
        return false;
    }

    if (!(buf[0] == 'I' && buf[1] == 'W' && buf[2] == 'i')) {
        setErr(errMsg, errSize, "Not an IWI file (bad magic)");
        free(buf);
        return false;
    }

    unsigned char version = buf[3];
    if (version != IWI_VERSION_BO1) {
        setErr(errMsg, errSize, "Unsupported IWI version");
        free(buf);
        return false;
    }

    int infoOffset = 0x04;
    int offsetsPos = 0x10;
    int offsetsCount = 8;

    int headerEnd = offsetsPos + offsetsCount * 4;
    if (fileSize < headerEnd) {
        setErr(errMsg, errSize, "File too small to be a valid IWI");
        free(buf);
        return false;
    }

    int format = buf[infoOffset];
    int width  = readU16(buf + infoOffset + 2);
    int height = readU16(buf + infoOffset + 4);

    if (width <= 0 || height <= 0 || width > 8192 || height > 8192) {
        setErr(errMsg, errSize, "Invalid image dimensions");
        free(buf);
        return false;
    }

    long curr = headerEnd;
    long bestOffset = -1;
    long bestSize = 0;
    for (int i = 0; i < offsetsCount; i++) {
        long off, size;
        long oi = (long)readU32(buf + offsetsPos + i * 4);
        if (i == 0) {
            off = oi;
            size = fileSize - oi;
        } else if (i == offsetsCount - 1) {
            off = curr;
            size = oi - curr;
        } else {
            long prev = (long)readU32(buf + offsetsPos + (i - 1) * 4);
            off = oi;
            size = prev - oi;
        }
        if (off >= curr && size > 0 && off + size <= fileSize && size > bestSize) {
            bestOffset = off;
            bestSize = size;
        }
    }

    if (bestOffset < 0) {
        setErr(errMsg, errSize, "Invalid mipmap offset");
        free(buf);
        return false;
    }

    const unsigned char *data = buf + bestOffset;
    size_t dataLen = (size_t)bestSize;

    unsigned char *pixels = (unsigned char *)malloc((size_t)width * height * 4);
    if (!pixels) {
        setErr(errMsg, errSize, "Out of memory");
        free(buf);
        return false;
    }

    bool ok = false;
    switch (format) {
        case IWI_FORMAT_DXT1: ok = decodeDXT1(data, dataLen, width, height, pixels); break;
        case IWI_FORMAT_DXT3: ok = decodeDXT3(data, dataLen, width, height, pixels); break;
        case IWI_FORMAT_DXT5: ok = decodeDXT5(data, dataLen, width, height, pixels); break;
        case IWI_FORMAT_ARGB32:
        case IWI_FORMAT_RGB24:
            ok = decodeUncompressed(data, dataLen, width, height, format, pixels);
            break;
        default:
            setErr(errMsg, errSize, "Unsupported IWI pixel format");
            break;
    }

    free(buf);

    if (!ok) {
        if (format == IWI_FORMAT_DXT1 || format == IWI_FORMAT_DXT3 ||
            format == IWI_FORMAT_DXT5 || format == IWI_FORMAT_ARGB32 ||
            format == IWI_FORMAT_RGB24) {
            setErr(errMsg, errSize, "Corrupt or truncated pixel data");
        }
        free(pixels);
        return false;
    }

    out->pixels = pixels;
    out->width = width;
    out->height = height;
    return true;
}

void iwiFree(IwiImage *img) {
    if (img && img->pixels) {
        free(img->pixels);
        img->pixels = NULL;
        img->width = 0;
        img->height = 0;
    }
}
