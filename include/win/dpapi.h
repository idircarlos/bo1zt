#ifndef WIN_DPAPI_H_
#define WIN_DPAPI_H_

#include <stdbool.h>
#include <stddef.h>

bool dpapiProtect(const void *data, size_t size, void **out, size_t *outSize);
bool dpapiUnprotect(const void *data, size_t size, void **out, size_t *outSize);
void dpapiFree(void *data);

#endif // WIN_DPAPI_H_
