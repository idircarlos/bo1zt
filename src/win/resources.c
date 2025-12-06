#include "win/resources.h"
#include <stdio.h>

#define MAX_EMBEDDED_FONTS 16

static HANDLE fontHandles[MAX_EMBEDDED_FONTS];
static int fontCount = 0;

void resourcesInit(void) {
    fontCount = 0;
    for (int i = 0; i < MAX_EMBEDDED_FONTS; i++) {
        fontHandles[i] = NULL;
    }
}


void resourcesCleanup(void) {
    // Cleanup fonts
    for (int i = 0; i < fontCount; i++) {
        if (fontHandles[i]) {
            RemoveFontMemResourceEx(fontHandles[i]);
            fontHandles[i] = NULL;
        }
    }
    fontCount = 0;
}

bool resourcesLoadFont(int resourceId) {
    if (fontCount >= MAX_EMBEDDED_FONTS) {
        return false;
    }

    HMODULE hModule = GetModuleHandleA(NULL);
    if (!hModule) {
        return false;
    }

    HRSRC hRes = FindResourceA(hModule, MAKEINTRESOURCEA(resourceId), RT_RCDATA);
    if (!hRes) {
        return false;
    }

    HGLOBAL hMem = LoadResource(hModule, hRes);
    if (!hMem) {
        return false;
    }

    void* pFontData = LockResource(hMem);
    if (!pFontData) {
        return false;
    }

    DWORD fontSize = SizeofResource(hModule, hRes);
    if (fontSize == 0) {
        return false;
    }

    DWORD numFonts = 0;
    HANDLE hFont = AddFontMemResourceEx(pFontData, fontSize, NULL, &numFonts);
    
    if (!hFont) {
        return false;
    }

    if (numFonts == 0) {
        RemoveFontMemResourceEx(hFont);
        return false;
    }

    fontHandles[fontCount++] = hFont;
    return true;
}

bool resourcesExtractToFile(int resourceId, const char* outputPath) {
    if (!outputPath) {
        return false;
    }

    HMODULE hModule = GetModuleHandle(NULL);
    if (!hModule) {
        return false;
    }

    HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hResource) {
        return false;
    }

    HGLOBAL hLoadedResource = LoadResource(hModule, hResource);
    if (!hLoadedResource) {
        return false;
    }

    void* pResourceData = LockResource(hLoadedResource);
    if (!pResourceData) {
        return false;
    }

    DWORD resourceSize = SizeofResource(hModule, hResource);
    if (resourceSize == 0) {
        return false;
    }

    FILE* outputFile = fopen(outputPath, "wb");
    if (!outputFile) {
        return false;
    }

    size_t written = fwrite(pResourceData, 1, resourceSize, outputFile);
    fclose(outputFile);

    return (written == resourceSize);
}
