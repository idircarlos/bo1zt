#include <windows.h>

#include "win/text.h"

#include <stdlib.h>

wchar_t *textToWide(const char *utf8) {
    if (!utf8) utf8 = "";

    int length = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (length <= 0) return NULL;

    wchar_t *wide = (wchar_t *)malloc((size_t)length * sizeof(wchar_t));
    if (!wide) return NULL;

    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide, length);
    return wide;
}
