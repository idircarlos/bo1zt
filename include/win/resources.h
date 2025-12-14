#ifndef RESOURCES_H
#define RESOURCES_H

#include <windows.h>
#include <stdbool.h>

void resourcesInit(void);
void resourcesCleanup(void);
bool resourcesLoadFont(int resourceId);
bool resourcesExtractToFile(int resourceId, const char* outputPath);
bool resourcesExtractZip(int resourceId, const char* outputDir);

#endif // RESOURCES_H
