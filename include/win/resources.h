#ifndef RESOURCES_H
#define RESOURCES_H

#include <windows.h>
#include <stdbool.h>
#include <stdint.h>

void resourcesInit(void);
void resourcesCleanup(void);
bool resourcesGetData(int resourceId, void** outData, uint32_t* outSize);
bool resourcesLoadFont(int resourceId);
bool resourcesExtractToFile(int resourceId, const char* outputPath);
bool resourcesExtractZip(int resourceId, const char* outputDir);
IStream* resourcesCreateStream(int resourceId);
void resourcesReleaseStream(IStream* stream);

#endif // RESOURCES_H
