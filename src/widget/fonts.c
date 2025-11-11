#include "fonts.h"
#include <windows.h>

#define MAX_EMBEDDED_FONTS 16

static HANDLE fontHandles[MAX_EMBEDDED_FONTS];
static int fontCount = 0;

void fontsInit(void) {
    fontCount = 0;
    for (int i = 0; i < MAX_EMBEDDED_FONTS; i++) {
        fontHandles[i] = NULL;
    }
}

void fontsCleanup(void) {
    for (int i = 0; i < fontCount; i++) {
        if (fontHandles[i]) {
            RemoveFontMemResourceEx(fontHandles[i]);
            fontHandles[i] = NULL;
        }
    }
    fontCount = 0;
}

bool fontsLoad(int resourceId) {
    if (fontCount >= MAX_EMBEDDED_FONTS) {
    return FALSE;
    }

    // Get the current module handle (the .exe itself)
    HMODULE hModule = GetModuleHandleA(NULL);
    if (!hModule) {
    return FALSE;
    }

    // Find the font resource (use numeric type RT_RCDATA to avoid name mismatch)
    HRSRC hRes = FindResourceA(hModule, MAKEINTRESOURCEA(resourceId), RT_RCDATA);
    if (!hRes) return FALSE;

    // Load the resource
    HGLOBAL hMem = LoadResource(hModule, hRes);
    if (!hMem) return FALSE;

    // Lock the resource to get a pointer to the font data
    void* pFontData = LockResource(hMem);
    if (!pFontData) return FALSE;

    // Get the size of the font data
    DWORD fontSize = SizeofResource(hModule, hRes);
    if (fontSize == 0) return FALSE;

    // Add the font to the system (private, not visible to other apps)
    DWORD numFonts = 0;
    HANDLE hFont = AddFontMemResourceEx(pFontData, fontSize, NULL, &numFonts);
    
    if (!hFont) return FALSE;

    if (numFonts == 0) { RemoveFontMemResourceEx(hFont); return FALSE; }

    // Store the handle for cleanup
    fontHandles[fontCount++] = hFont;
    
    return TRUE;
}
