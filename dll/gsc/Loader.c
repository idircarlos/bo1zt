#include <Windows.h>
#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Loader.h"
#include "miniz.h"
#include "cdl.h"
#include "../utils/Log.h"

#define T5_Scr_LoadScript         0x00661AF0
#define T5_Scr_GetFunctionHandle  0x004E3470
#define T5_DB_LinkXAssetEntry     0x007A2F10
#define T5_Scr_ExecThread         0x005598E0
#define T5_Scr_FreeThread         0x005DE2C0
#define T5_Scr_LoadGameType       0x004B7F80
#define T5_Dvar_FindVar           0x0057FF80
#define T5_Assign_Hotfix          0x007A4800
#define T5_init_trigger           0x00C793B0
#define T5_Thread_Timer           0x004C06E0

#define GSC_SIZE_BUF (1024 * 1024)
#define GSC_SIZE_PATH 256

// Function pointers
static Scr_LoadScript_t Scr_LoadScript = (Scr_LoadScript_t)T5_Scr_LoadScript;
static Scr_GetFunctionHandle_t Scr_GetFunctionHandle = (Scr_GetFunctionHandle_t)T5_Scr_GetFunctionHandle;
static Scr_ExecThread_t Scr_ExecThread = (Scr_ExecThread_t)T5_Scr_ExecThread;
static Scr_FreeThread_t Scr_FreeThread = (Scr_FreeThread_t)T5_Scr_FreeThread;
static Scr_LoadGameType_t Scr_LoadGameType = (Scr_LoadGameType_t)T5_Scr_LoadGameType;
static Dvar_FindVar_t Dvar_FindVar = (Dvar_FindVar_t)T5_Dvar_FindVar;
static DB_LinkXAssetEntry_t DB_LinkXAssetEntry = (DB_LinkXAssetEntry_t)T5_DB_LinkXAssetEntry;
static Assign_Hotfix_t Assign_Hotfix = (Assign_Hotfix_t)T5_Assign_Hotfix;
static Thread_Timer_t Thread_Timer = (Thread_Timer_t)T5_Thread_Timer;

// Global variables
static int* initTrigger = (int*)T5_init_trigger;
static uint8_t modDir[GSC_SIZE_PATH];
static uint8_t entryFile[GSC_SIZE_PATH + 16];
static int funcHandle = 0;

// Order matters since scripts depends on others!
static const char* gscScripts[] = {
    "api/static_box.gsc",
    "api/perks.gsc",
    "api.gsc",
    "listeners.gsc",
    "workers.gsc",
    "main.gsc",
};

static const size_t gscScriptsCount = sizeof(gscScripts) / sizeof(gscScripts[0]);

// Aux
static void stripExt(uint8_t* fname) {
    uint8_t* end = fname + strlen((char*)fname);
    while (end > fname && *end != '.') --end;
    if (end > fname) *end = '\0';
}

static BOOL FileExists(LPCSTR szPath) {
    DWORD attrib = GetFileAttributesA(szPath);
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

static BOOL DirectoryExists(LPCSTR szPath) {
    DWORD attrib = GetFileAttributesA(szPath);
    return (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY));
}

// Hotfix to prevent crash when exiting gamemode
static int* __cdecl Assign_Hotfix_hk(int* a1, int* a2) {
    if (a1 != NULL && a2 != NULL) {
        return Assign_Hotfix(a1, a2);
    }
    return 0;
}

// AC bypass to allow detouring
static void __cdecl Thread_Timer_hk(uint8_t a1, int a2, int a3, int a4) {
    (void)a1; (void)a2; (void)a3; (void)a4;
    return;
}

// Load custom GSC script
static int32_t __cdecl Scr_LoadScript_hk(int32_t scriptInstance, const uint8_t* scriptName) {
    LOG_INFO("[GSC] Loading script %s", scriptName);
    int result = Scr_LoadScript(scriptInstance, scriptName);

    uint8_t mapnameBuffer[GSC_SIZE_PATH];
    uint8_t* mapname = Dvar_FindVar((uint8_t*)"mapname");

    sprintf((char*)mapnameBuffer, "maps/%s", mapname);

    if (strcmp((char*)mapnameBuffer, (char*)scriptName) == 0 && strcmp((char*)mapnameBuffer, "maps/frontend") != 0) {
        for (int i = 0; i < gscScriptsCount; i++) {
            snprintf((char*)entryFile, sizeof(entryFile), "%s/%s", modDir, gscScripts[i]);

            if (!FileExists((char*)entryFile)) {
                LOG_WARN("[GSC] Script not found: %s", entryFile);
                continue;
            }

            LOG_INFO("[GSC] Loading bo1zt script: %s", entryFile);

            FILE* gscScript = fopen((char*)entryFile, "rb");
            if (!gscScript) continue;

            fseek(gscScript, 0, SEEK_END);
            uint32_t gscSize = ftell(gscScript);
            fseek(gscScript, 0, SEEK_SET);

            uint8_t* sourceBuffer = (uint8_t*)malloc(gscSize + 1);
            if (!sourceBuffer) { fclose(gscScript); continue; }

            fread(sourceBuffer, 1, gscSize, gscScript);
            sourceBuffer[gscSize] = 0;

            mz_ulong gscCompressSize = mz_compressBound(gscSize + 1);
            uint8_t* dataBuffer = (uint8_t*)malloc(gscCompressSize + sizeof(RawFileData));
            if (!dataBuffer) { free(sourceBuffer); fclose(gscScript); continue; }

            uint8_t* compressBuffer = (uint8_t*)(dataBuffer + sizeof(RawFileData));
            int cmpStatus = mz_compress(compressBuffer, &gscCompressSize, sourceBuffer, gscSize + 1);
            free(sourceBuffer);

            if (cmpStatus == MZ_OK) {
                RawFileData* fileData = (RawFileData*)dataBuffer;
                fileData->compressedSize = gscCompressSize;
                fileData->deflatedSize = gscSize + 1;

                XAsset entry = { 0 };
                entry.type = ASSET_TYPE_RAWFILE;
                entry.header.rawFile = (RawFile*)malloc(sizeof(RawFile));
                entry.header.rawFile->buffer = dataBuffer;
                entry.header.rawFile->len = gscCompressSize + sizeof(RawFileData);
                entry.header.rawFile->name = entryFile;

                DB_LinkXAssetEntry(&entry, 0);

                uint8_t scriptFile[GSC_SIZE_PATH];
                snprintf((char*)scriptFile, sizeof(scriptFile), "%s", entryFile);
                stripExt(scriptFile);

                Scr_LoadScript(0, scriptFile);

                free(dataBuffer);
                free(entry.header.rawFile);

                if (strcmp((char*)scriptFile, "bo1zt/gsc/main") == 0) {
                    funcHandle = Scr_GetFunctionHandle(0, scriptFile, (uint8_t*)"main");
                    if (funcHandle) {
                        LOG_INFO("[GSC] main handle for %s @ %p", scriptFile, (void*)(uintptr_t)funcHandle);
                    }
                }
            }
            fclose(gscScript);
        }
    }

    return result;
}

// Execute function handle at correct timing
static void __cdecl Scr_LoadGameType_hk(void) {
    Scr_LoadGameType();
    uint8_t* mapname = Dvar_FindVar((uint8_t*)"mapname");

    if (strcmp((char*)mapname, "frontend") == 0) {
        LOG_INFO("[GSC] Loaded main menu");
    } else if (funcHandle > 0) {
        LOG_INFO("[GSC] Executing main @ %p", (void*)(uintptr_t)funcHandle);
        int16_t handle = Scr_ExecThread(0, funcHandle, 0);
        Scr_FreeThread(handle, 0);
    }
}

// Recursively make directory
static void mkdirRecursive(uint8_t* path) {
    if (path == NULL) return;
    
    uint8_t* buffer = (uint8_t*)malloc(GSC_SIZE_PATH);
    if (!buffer) {
        LOG_ERROR("[GSC] Failed allocating memory for mkdir");
        return;
    }

    int lenPath = strlen((char*)path);
    for (int i = 0; i < lenPath; i++) {
        if (path[i] == '/') {
            strncpy((char*)buffer, (char*)path, i);
            buffer[i] = 0;
            _mkdir((char*)buffer);
        }
    }
    free(buffer);
}

// Dump GSC and CSC scripts as they are loaded
static XAssetEntryPoolEntry* __cdecl DB_LinkXAssetEntry_hk(XAsset* newEntry, int32_t allowOverride) {
    struct RawFile* rawfile = newEntry->header.rawFile;
    uint8_t* buffer = (uint8_t*)malloc(GSC_SIZE_BUF);
    uint8_t* path = (uint8_t*)malloc(GSC_SIZE_PATH);
    uint8_t* scriptName = NULL;
    z_stream stream;
    int status = 0;
    int inflateLen = 0;

    if (!buffer || !path) {
        LOG_ERROR("[GSC] Failed allocating memory for dump");
        if (buffer) free(buffer);
        if (path) free(path);
        return DB_LinkXAssetEntry(newEntry, allowOverride);
    }

    if (newEntry->type == ASSET_TYPE_RAWFILE && rawfile != NULL) {
        scriptName = rawfile->name;

        if (strstr((char*)scriptName, ".gsc") != NULL || strstr((char*)scriptName, ".csc") != NULL) {
            memset(&stream, 0, sizeof(stream));
            const uint8_t* data = (const uint8_t*)(rawfile->buffer + GSC_FILE_OFFSET);

            stream.next_in = (Bytef*)data;
            stream.avail_in = rawfile->len - GSC_FILE_OFFSET;
            stream.next_out = buffer;
            stream.avail_out = GSC_SIZE_BUF;

            inflateInit(&stream);
            status = inflate(&stream, Z_SYNC_FLUSH);
            inflateEnd(&stream);
            inflateLen = stream.total_out;

            if (status == Z_STREAM_END || status == Z_OK) {
                sprintf((char*)path, "%s/%s", GSC_PATH_DUMP, scriptName);
                LOG_INFO("[GSC] Cache: %s", path);
                mkdirRecursive(path);
                FILE* inflatedScript = fopen((char*)path, "wb");
                if (inflatedScript) {
                    fwrite(buffer, sizeof(uint8_t), inflateLen, inflatedScript);
                    fclose(inflatedScript);
                }
            }
        }
    }

    free(buffer);
    free(path);
    return DB_LinkXAssetEntry(newEntry, allowOverride);
}


// GSC Init thread
static DWORD WINAPI GSCInitThread(LPVOID lpParam) {
    (void)lpParam;

    // Wait until all threads have started
    while (!*initTrigger) { Sleep(10); }

    LOG_INFO("[GSC] Loader init");
    _mkdir(GSC_PATH_ROOT);

    // Fixed paths: gsc/main.gsc
    snprintf((char*)modDir, sizeof(modDir), "%s/gsc", GSC_PATH_ROOT);
    snprintf((char*)entryFile, sizeof(entryFile), "%s/main.gsc", modDir);

    if (!DirectoryExists((char*)modDir)) {
        LOG_WARN("[GSC] Mod directory does not exist: %s", modDir);
        return EXIT_SUCCESS;
    }

    LOG_INFO("[GSC] Mod directory found: %s", modDir);

    if (!FileExists((char*)entryFile)) {
        LOG_WARN("[GSC] Mod entry file does not exist: %s", entryFile);
        return EXIT_SUCCESS;
    }

    LOG_INFO("[GSC] Mod entry found: %s", entryFile);
    LOG_INFO("[GSC] Applying detours...");

    // Setup hooks
    struct cdl_jmp_patch threadTimer = cdl_jmp_attach((void**)&Thread_Timer, Thread_Timer_hk);
    struct cdl_jmp_patch assignHotfix = cdl_jmp_attach((void**)&Assign_Hotfix, Assign_Hotfix_hk);
    struct cdl_jmp_patch linkAsset = cdl_jmp_attach((void**)&DB_LinkXAssetEntry, DB_LinkXAssetEntry_hk);
    struct cdl_jmp_patch loadScript = cdl_jmp_attach((void**)&Scr_LoadScript, Scr_LoadScript_hk);
    struct cdl_jmp_patch loadGameType = cdl_jmp_attach((void**)&Scr_LoadGameType, Scr_LoadGameType_hk);

    LOG_INFO("[GSC] Thread_Timer detour applied");
    cdl_jmp_dbg(&threadTimer);
    LOG_INFO("[GSC] Assign_Hotfix detour applied");
    cdl_jmp_dbg(&assignHotfix);
    LOG_INFO("[GSC] DB_LinkXAssetEntry detour applied");
    cdl_jmp_dbg(&linkAsset);
    LOG_INFO("[GSC] Scr_LoadScript detour applied");
    cdl_jmp_dbg(&loadScript);
    LOG_INFO("[GSC] Scr_LoadGameType detour applied");
    cdl_jmp_dbg(&loadGameType);
    LOG_INFO("[GSC] All detours applied successfully");

    return EXIT_SUCCESS;
}

// Initialize GSC loader
void GSCInit(void) {
    CreateThread(NULL, 0, GSCInitThread, NULL, 0, NULL);
}
