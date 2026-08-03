#include <windows.h>
#include <wincrypt.h>

#include "win/dpapi.h"
#include "logger.h"

bool dpapiProtect(const void *data, size_t size, void **out, size_t *outSize) {
    if (!data || size == 0 || !out || !outSize) return false;

    DATA_BLOB plain = { (DWORD)size, (BYTE *)data };
    DATA_BLOB encrypted = {};
    if (!CryptProtectData(&plain, NULL, NULL, NULL, NULL,
                          CRYPTPROTECT_UI_FORBIDDEN, &encrypted)) {
        LOG_ERROR("DPAPI: CryptProtectData failed (%lu)", GetLastError());
        return false;
    }

    *out = encrypted.pbData;
    *outSize = encrypted.cbData;
    return true;
}

bool dpapiUnprotect(const void *data, size_t size, void **out, size_t *outSize) {
    if (!data || size == 0 || !out || !outSize) return false;

    DATA_BLOB encrypted = { (DWORD)size, (BYTE *)data };
    DATA_BLOB plain = {};
    if (!CryptUnprotectData(&encrypted, NULL, NULL, NULL, NULL,
                            CRYPTPROTECT_UI_FORBIDDEN, &plain)) {
        LOG_ERROR("DPAPI: CryptUnprotectData failed (%lu)", GetLastError());
        return false;
    }

    *out = plain.pbData;
    *outSize = plain.cbData;
    return true;
}

void dpapiFree(void *data) {
    LocalFree(data);
}
