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
#define GSC_SIZE_PATH 512

#define GSC_APP_FOLDER "bo1zt"
#define GSC_APPDATA_SCRIPTS "gsc"
#define GSC_APPDATA_DUMP "dump"

// Asset names mirror the layout under the scripts folder, so a file path is its GSC path
#define GSC_SCRIPT_ROOT "bo1zt"
#define GSC_SCRIPT_MODS_ROOT GSC_SCRIPT_ROOT "/mods"
#define GSC_SCRIPT_ENTRY GSC_SCRIPT_ROOT "/core/main"

#define GSC_FILE_OFFSET 0x8
#define GSC_SCRIPT_MAX 128
#define GSC_SCRIPT_EXTENSION ".gsc"
#define GSC_MOD_ENTRY "main"

// Debug only: inflates every stock script to the dump directory on each map load
#define GSC_DUMP_NATIVE_SCRIPTS 0

static Scr_LoadScript_t Scr_LoadScript = (Scr_LoadScript_t)T5_Scr_LoadScript;
static Scr_GetFunctionHandle_t Scr_GetFunctionHandle = (Scr_GetFunctionHandle_t)T5_Scr_GetFunctionHandle;
static Scr_ExecThread_t Scr_ExecThread = (Scr_ExecThread_t)T5_Scr_ExecThread;
static Scr_FreeThread_t Scr_FreeThread = (Scr_FreeThread_t)T5_Scr_FreeThread;
static Scr_LoadGameType_t Scr_LoadGameType = (Scr_LoadGameType_t)T5_Scr_LoadGameType;
static Dvar_FindVar_t Dvar_FindVar = (Dvar_FindVar_t)T5_Dvar_FindVar;
static DB_LinkXAssetEntry_t DB_LinkXAssetEntry = (DB_LinkXAssetEntry_t)T5_DB_LinkXAssetEntry;
static Assign_Hotfix_t Assign_Hotfix = (Assign_Hotfix_t)T5_Assign_Hotfix;
static Thread_Timer_t Thread_Timer = (Thread_Timer_t)T5_Thread_Timer;

typedef struct GscScript {
    char assetName[GSC_SIZE_PATH];
    bool execMain;
    int handle;
} GscScript;

static int* initTrigger = (int*)T5_init_trigger;
static char scriptDir[GSC_SIZE_PATH];
static char dumpDir[GSC_SIZE_PATH];

static GscScript scripts[GSC_SCRIPT_MAX];
static size_t scriptCount = 0;

static const char* nativeScripts[] = {
    // Kino
    "maps/zombie_theater_amb",
    "character/c_usa_dempsey_zt",
    "character/c_rus_nikolai_zt",
    "character/c_jap_takeo_zt",
    "character/c_ger_richtofen_zt",
    // Five
    "maps/zombie_pentagon_amb",
    // Ascension
    "maps/zombie_cosmodrome_amb",
    "character/c_usa_dempsey_dlc2",
    "character/c_rus_nikolai_dlc2",
    "character/c_jap_takeo_dlc2",
    "character/c_ger_richtofen_dlc2",
    // Moon
    "character/c_usa_dempsey_dlc5",
    "character/c_rus_nikolai_dlc5",
    "character/c_jap_takeo_dlc5",
    "character/c_ger_richtofen_dlc5",
    // Call of the Dead
    "character/c_zom_sarah_michelle_gellar_player",
    "character/c_zom_robert_englund_player",
    "character/c_zom_danny_trejo_player",
    "character/c_zom_michael_rooker_player",
    // WaW Classics
    "maps/zombie_cod5_sumpf",
    "maps/zombie_cod5_asylum",
    "maps/zombie_cod5_factory",
    // Common
    "maps/_zombiemode_weap_black_hole_bomb",
    "maps/_zombiemode_weap_nesting_dolls",
    "maps/_zombiemode_weap_quantum_bomb",
};

static const size_t nativeScriptCount = sizeof(nativeScripts) / sizeof(nativeScripts[0]);

static void stripExt(char* fileName) {
    char* dot = strrchr(fileName, '.');
    if (dot) *dot = '\0';
}

static bool fileExists(const char* path) {
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

static bool appFolderPath(char* out, size_t size, const char* subPath) {
    char appData[GSC_SIZE_PATH];
    if (!GetEnvironmentVariableA("APPDATA", appData, sizeof(appData))) return false;

    for (char* p = appData; *p; ++p) {
        if (*p == '\\') *p = '/';
    }

    int written = snprintf(out, size, "%s/%s/%s", appData, GSC_APP_FOLDER, subPath);
    return written > 0 && (size_t)written < size;
}

static void createParentDirs(const char* path) {
    char buffer[GSC_SIZE_PATH];
    snprintf(buffer, sizeof(buffer), "%s", path);

    for (char* p = buffer; *p; ++p) {
        if (*p != '/') continue;
        *p = '\0';
        _mkdir(buffer);
        *p = '/';
    }
}

static const char* currentMapName(void) {
    const char* mapName = (const char*)Dvar_FindVar((uint8_t*)"mapname");
    return mapName ? mapName : "";
}

static bool isFrontend(void) {
    return strcmp(currentMapName(), "frontend") == 0;
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
}

// Size includes the terminator, which the compiler expects in the inflated rawfile
static uint8_t* readSource(const char* filePath, uint32_t* outSize) {
    FILE* file = fopen(filePath, "rb");
    if (!file) return NULL;

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t* source = (uint8_t*)malloc((size_t)size + 1);
    size_t read = fread(source, 1, (size_t)size, file);
    source[read] = 0;
    fclose(file);

    *outSize = (uint32_t)read + 1;
    return source;
}

// The asset pool keeps these pointers, so name, header and buffer are never freed
static bool linkScriptAsset(const char* filePath, const char* assetName) {
    uint32_t sourceSize = 0;
    uint8_t* source = readSource(filePath, &sourceSize);
    if (!source) {
        LOG_WARN("[GSC] Unreadable script: %s", filePath);
        return false;
    }

    mz_ulong compressedSize = mz_compressBound(sourceSize);
    uint8_t* data = (uint8_t*)malloc(compressedSize + sizeof(RawFileData));
    int status = mz_compress(data + sizeof(RawFileData), &compressedSize, source, sourceSize);
    free(source);

    if (status != MZ_OK) {
        LOG_ERROR("[GSC] Failed compressing %s", filePath);
        free(data);
        return false;
    }

    RawFileData* header = (RawFileData*)data;
    header->compressedSize = compressedSize;
    header->deflatedSize = sourceSize;

    char* name = (char*)malloc(GSC_SIZE_PATH);
    snprintf(name, GSC_SIZE_PATH, "%s.gsc", assetName);

    RawFile* rawFile = (RawFile*)malloc(sizeof(RawFile));
    rawFile->name = (uint8_t*)name;
    rawFile->len = compressedSize + sizeof(RawFileData);
    rawFile->buffer = data;

    XAsset entry = { 0 };
    entry.type = ASSET_TYPE_RAWFILE;
    entry.header.rawFile = rawFile;
    DB_LinkXAssetEntry(&entry, 0);

    LOG_INFO("[GSC] Linked script: %s", name);
    return true;
}

static void addScript(const char* filePath, const char* assetName, bool execMain) {
    if (scriptCount >= GSC_SCRIPT_MAX) {
        LOG_WARN("[GSC] Script limit of %d reached, skipping %s", GSC_SCRIPT_MAX, assetName);
        return;
    }
    if (!linkScriptAsset(filePath, assetName)) return;

    GscScript* script = &scripts[scriptCount++];
    snprintf(script->assetName, sizeof(script->assetName), "%s", assetName);
    script->execMain = execMain;
    script->handle = 0;
}

static void loadNativeScripts(void) {
    for (size_t i = 0; i < nativeScriptCount; i++) {
        LOG_INFO("[GSC] Loading native script: %s", nativeScripts[i]);
        Scr_LoadScript(0, (const uint8_t*)nativeScripts[i]);
    }
}

static bool isScriptFile(const char* fileName) {
    const char* dot = strrchr(fileName, '.');
    return dot && _stricmp(dot, GSC_SCRIPT_EXTENSION) == 0;
}

// A mod is a folder under mods/ and only its main.gsc is executed
static bool isModEntry(const char* assetName) {
    const size_t rootLength = sizeof(GSC_SCRIPT_MODS_ROOT "/") - 1;
    if (strncmp(assetName, GSC_SCRIPT_MODS_ROOT "/", rootLength) != 0) return false;

    const char* slash = strchr(assetName + rootLength, '/');
    return slash && _stricmp(slash + 1, GSC_MOD_ENTRY) == 0;
}

static void linkScriptDir(const char* dir, const char* prefix) {
    char pattern[GSC_SIZE_PATH];
    snprintf(pattern, sizeof(pattern), "%s/*", dir);

    WIN32_FIND_DATAA found;
    HANDLE search = FindFirstFileA(pattern, &found);
    if (search == INVALID_HANDLE_VALUE) return;

    do {
        if (strcmp(found.cFileName, ".") == 0 || strcmp(found.cFileName, "..") == 0) continue;

        char assetName[GSC_SIZE_PATH];
        char filePath[GSC_SIZE_PATH];

        snprintf(assetName, sizeof(assetName), "%s/%s", prefix, found.cFileName);
        snprintf(filePath, sizeof(filePath), "%s/%s", dir, found.cFileName);

        if (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            linkScriptDir(filePath, assetName);
            continue;
        }

        if (!isScriptFile(found.cFileName)) continue;

        stripExt(assetName);
        addScript(filePath, assetName, strcmp(assetName, GSC_SCRIPT_ENTRY) == 0 || isModEntry(assetName));
    } while (FindNextFileA(search, &found));

    FindClose(search);
}

static void linkScripts(void) {
    char rootDir[GSC_SIZE_PATH];
    snprintf(rootDir, sizeof(rootDir), "%s/%s", scriptDir, GSC_SCRIPT_ROOT);
    linkScriptDir(rootDir, GSC_SCRIPT_ROOT);
}

static void compileScripts(void) {
    for (size_t i = 0; i < scriptCount; i++) {
        GscScript* script = &scripts[i];
        Scr_LoadScript(0, (const uint8_t*)script->assetName);

        if (!script->execMain) continue;

        script->handle = Scr_GetFunctionHandle(0, (const uint8_t*)script->assetName, (const uint8_t*)"main");
        if (script->handle) {
            LOG_INFO("[GSC] main handle for %s @ %p", script->assetName, (void*)(uintptr_t)script->handle);
        } else {
            LOG_WARN("[GSC] Script has no main(): %s", script->assetName);
        }
    }
}

// Every asset is linked before the first compile, so scripts may reference each other in any order
static void loadScripts(void) {
    loadNativeScripts();

    scriptCount = 0;
    linkScripts();
    compileScripts();
}

static void execScripts(void) {
    for (size_t i = 0; i < scriptCount; i++) {
        int handle = scripts[i].handle;
        if (handle <= 0) continue;

        LOG_INFO("[GSC] Executing main of %s @ %p", scripts[i].assetName, (void*)(uintptr_t)handle);
        uint16_t thread = Scr_ExecThread(0, handle, 0);
        Scr_FreeThread(thread, 0);
    }
}

// Piggyback the map script load to inject ours into the same compile pass
static int32_t __cdecl Scr_LoadScript_hk(int32_t scriptInstance, const uint8_t* scriptName) {
    LOG_INFO("[GSC] Loading script %s", scriptName);
    int32_t result = Scr_LoadScript(scriptInstance, scriptName);

    if (isFrontend()) return result;

    char mapScript[GSC_SIZE_PATH];
    snprintf(mapScript, sizeof(mapScript), "maps/%s", currentMapName());

    if (strcmp(mapScript, (const char*)scriptName) == 0) {
        loadScripts();
    }

    return result;
}

// Execute function handles at correct timing
static void __cdecl Scr_LoadGameType_hk(void) {
    Scr_LoadGameType();

    if (isFrontend()) {
        LOG_INFO("[GSC] Loaded main menu");
        return;
    }

    execScripts();
}

static void dumpRawFile(const RawFile* rawFile) {
    uint8_t* buffer = (uint8_t*)malloc(GSC_SIZE_BUF);
    if (!buffer) {
        LOG_ERROR("[GSC] Failed allocating memory for dump");
        return;
    }

    z_stream stream = { 0 };
    stream.next_in = (Bytef*)(rawFile->buffer + GSC_FILE_OFFSET);
    stream.avail_in = rawFile->len - GSC_FILE_OFFSET;
    stream.next_out = buffer;
    stream.avail_out = GSC_SIZE_BUF;

    inflateInit(&stream);
    int status = inflate(&stream, Z_SYNC_FLUSH);
    int inflatedSize = stream.total_out;
    inflateEnd(&stream);

    if (status == Z_STREAM_END || status == Z_OK) {
        char path[GSC_SIZE_PATH];
        snprintf(path, sizeof(path), "%s/%s", dumpDir, (const char*)rawFile->name);
        LOG_INFO("[GSC] Dump: %s", path);

        createParentDirs(path);
        FILE* dumped = fopen(path, "wb");
        if (dumped) {
            fwrite(buffer, sizeof(uint8_t), inflatedSize, dumped);
            fclose(dumped);
        }
    }

    free(buffer);
}

// Dump GSC and CSC scripts as they are loaded
static XAssetEntryPoolEntry* __cdecl DB_LinkXAssetEntry_hk(XAsset* newEntry, int32_t allowOverride) {
    RawFile* rawFile = newEntry->header.rawFile;

    if (newEntry->type == ASSET_TYPE_RAWFILE && rawFile) {
        const char* name = (const char*)rawFile->name;
        if (GSC_DUMP_NATIVE_SCRIPTS && (strstr(name, ".gsc") || strstr(name, ".csc"))) {
            dumpRawFile(rawFile);
        }
    }

    return DB_LinkXAssetEntry(newEntry, allowOverride);
}

static void attachDetour(void** target, void* detour, const char* name) {
    struct cdl_jmp_patch patch = cdl_jmp_attach(target, detour);
    LOG_INFO("[GSC] %s detour applied", name);
    cdl_jmp_dbg(&patch);
}

static DWORD WINAPI GSCInitThread(LPVOID lpParam) {
    (void)lpParam;

    // Wait until all threads have started
    while (!*initTrigger) { Sleep(10); }

    LOG_INFO("[GSC] Loader init");

    if (!appFolderPath(scriptDir, sizeof(scriptDir), GSC_APPDATA_SCRIPTS) ||
        !appFolderPath(dumpDir, sizeof(dumpDir), GSC_APPDATA_DUMP)) {
        LOG_ERROR("[GSC] Failed to resolve %%APPDATA%%");
        return EXIT_FAILURE;
    }

    char entryFile[GSC_SIZE_PATH];
    snprintf(entryFile, sizeof(entryFile), "%s/%s.gsc", scriptDir, GSC_SCRIPT_ENTRY);

    if (!fileExists(entryFile)) {
        LOG_WARN("[GSC] Trainer entry file does not exist: %s", entryFile);
        return EXIT_SUCCESS;
    }

    LOG_INFO("[GSC] Trainer entry found: %s", entryFile);
    LOG_INFO("[GSC] Dump directory: %s", dumpDir);
    LOG_INFO("[GSC] Applying detours...");

    attachDetour((void**)&Thread_Timer, Thread_Timer_hk, "Thread_Timer");
    attachDetour((void**)&Assign_Hotfix, Assign_Hotfix_hk, "Assign_Hotfix");
    attachDetour((void**)&DB_LinkXAssetEntry, DB_LinkXAssetEntry_hk, "DB_LinkXAssetEntry");
    attachDetour((void**)&Scr_LoadScript, Scr_LoadScript_hk, "Scr_LoadScript");
    attachDetour((void**)&Scr_LoadGameType, Scr_LoadGameType_hk, "Scr_LoadGameType");

    LOG_INFO("[GSC] All detours applied successfully");

    return EXIT_SUCCESS;
}

void GSCInit(void) {
    CreateThread(NULL, 0, GSCInitThread, NULL, 0, NULL);
}
