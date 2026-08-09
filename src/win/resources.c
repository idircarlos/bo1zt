#include "win/resources.h"
#include "win/file.h"
#include <stdio.h>
#include <string.h>
#include "miniz.h"

// IStream vtable for C
typedef struct IStreamVtbl_ {
    HRESULT(__stdcall* QueryInterface)(IStream*, REFIID, void**);
    ULONG(__stdcall* AddRef)(IStream*);
    ULONG(__stdcall* Release)(IStream*);
} IStreamVtbl_;

#define STREAM_RELEASE(pStream) (((IStreamVtbl_*)(*(void**)(pStream)))->Release(pStream))

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

    void* pFontData = NULL;
    uint32_t fontSize = 0;
    if (!resourcesGetData(resourceId, &pFontData, &fontSize)) {
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

    void* pResourceData = NULL;
    uint32_t resourceSize = 0;
    if (!resourcesGetData(resourceId, &pResourceData, &resourceSize)) {
        return false;
    }

    return fileWriteAll(outputPath, pResourceData, resourceSize);
}

bool resourcesExtractZip(int resourceId, const char* outputDir) {
    if (!outputDir) return false;

    void* pResourceData = NULL;
    uint32_t resourceSize = 0;
    if (!resourcesGetData(resourceId, &pResourceData, &resourceSize)) {
        return false;
    }

    mz_zip_archive zip = {0};
    if (!mz_zip_reader_init_mem(&zip, pResourceData, resourceSize, 0)) {
        return false;
    }

    fileCreateFolder(outputDir);

    int numFiles = (int)mz_zip_reader_get_num_files(&zip);
    for (int i = 0; i < numFiles; i++) {
        mz_zip_archive_file_stat fileStat;
        if (!mz_zip_reader_file_stat(&zip, i, &fileStat)) continue;

        char fullPath[MAX_PATH];
        snprintf(fullPath, MAX_PATH, "%s/%s", outputDir, fileStat.m_filename);

        if (mz_zip_reader_is_file_a_directory(&zip, i)) {
            fileCreateFolder(fullPath);
        } else {
            char* slash = strrchr(fullPath, '/');
            char* backslash = strrchr(fullPath, '\\');
            char* sep = slash > backslash ? slash : backslash;
            if (sep) {
                *sep = '\0';
                fileCreateFolder(fullPath);
                *sep = '/';
            }
            mz_zip_reader_extract_to_file(&zip, i, fullPath, 0);
        }
    }

    mz_zip_reader_end(&zip);
    return true;
}


bool resourcesGetData(int resourceId, void** outData, uint32_t* outSize) {
    if (!outData || !outSize) return false;

    HMODULE hModule = GetModuleHandle(NULL);
    if (!hModule) return false;

    HRSRC hRes = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hRes) return false;

    HGLOBAL hData = LoadResource(hModule, hRes);
    if (!hData) return false;

    *outData = LockResource(hData);
    *outSize = SizeofResource(hModule, hRes);

    return (*outData != NULL && *outSize > 0);
}

IStream* resourcesCreateStream(int resourceId) {
    void* pData = NULL;
    uint32_t dataSize = 0;

    if (!resourcesGetData(resourceId, &pData, &dataSize)) {
        return NULL;
    }

    // Copy to moveable memory for IStream
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, dataSize);
    if (!hMem) return NULL;

    void* pMem = GlobalLock(hMem);
    memcpy(pMem, pData, dataSize);
    GlobalUnlock(hMem);

    IStream* pStream = NULL;
    if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) != S_OK) {
        GlobalFree(hMem);
        return NULL;
    }

    return pStream;
}

void resourcesReleaseStream(IStream* stream) {
    if (stream) {
        STREAM_RELEASE(stream);
    }
}
