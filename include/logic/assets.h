#ifndef LOGIC_ASSETS_H_
#define LOGIC_ASSETS_H_

#include <stdbool.h>
#include <stddef.h>

bool assetsInstalled(void);
int assetsZoneCount(void);
const char *assetsZoneName(int index);
bool assetsExtractZone(const char *gameLocation, int index);
void assetsCleanup(void);
bool assetsModelExportDir(char *out, size_t size);
int assetsModelTotal(void);
int assetsExtractedCount(void);

#endif // LOGIC_ASSETS_H_
