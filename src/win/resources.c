#include "win/resources.h"
#include <stdio.h>
#include <direct.h>
#include <string.h>
#include "miniz.h"

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

static void mkdirRecursive(const char* path) {
    char tmp[MAX_PATH];
    strncpy(tmp, path, MAX_PATH - 1);
    tmp[MAX_PATH - 1] = '\0';
    
    for (char* p = tmp + 1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            *p = '\0';
            _mkdir(tmp);
            *p = '/';
        }
    }
}

bool resourcesExtractZip(int resourceId, const char* outputDir) {
    if (!outputDir) return false;

    HMODULE hModule = GetModuleHandle(NULL);
    if (!hModule) return false;

    HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hResource) return false;

    HGLOBAL hLoadedResource = LoadResource(hModule, hResource);
    if (!hLoadedResource) return false;

    void* pResourceData = LockResource(hLoadedResource);
    if (!pResourceData) return false;

    DWORD resourceSize = SizeofResource(hModule, hResource);
    if (resourceSize == 0) return false;

    mz_zip_archive zip = {0};
    if (!mz_zip_reader_init_mem(&zip, pResourceData, resourceSize, 0)) {
        return false;
    }

    _mkdir(outputDir);

    int numFiles = (int)mz_zip_reader_get_num_files(&zip);
    for (int i = 0; i < numFiles; i++) {
        mz_zip_archive_file_stat fileStat;
        if (!mz_zip_reader_file_stat(&zip, i, &fileStat)) continue;

        char fullPath[MAX_PATH];
        snprintf(fullPath, MAX_PATH, "%s/%s", outputDir, fileStat.m_filename);

        if (mz_zip_reader_is_file_a_directory(&zip, i)) {
            _mkdir(fullPath);
        } else {
            mkdirRecursive(fullPath);
            mz_zip_reader_extract_to_file(&zip, i, fullPath, 0);
        }
    }

    mz_zip_reader_end(&zip);
    return true;
}
