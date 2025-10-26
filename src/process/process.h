#ifndef PROCESS_H_
#define PROCESS_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct Process Process;

Process *processOpen(const char *executableName);
bool processIsRunning(const char *executableName);
bool processIsWindowAttached(Process *process);
bool processTryAttachWindow(Process *process, const char *windowTitle);
bool processIsBorderless(Process *process);
bool processMakeBorderless(Process *process, bool enabled);
void processWaitUntilCloses(Process *process);
void processClose(Process *process);
bool processRead(Process *process, uint32_t address, void *buffer, size_t size);
bool processWrite(Process *process, uint32_t address, const void *buffer, size_t size);
bool processAllocatePage(Process *process, size_t size, uintptr_t *address);
bool processVirtualProtect(Process *process, uint32_t address, size_t size, uint32_t protect, uint32_t *oldProtect);
bool processFindPattern(Process *process, uintptr_t startAddress, size_t regionSize, const uint8_t *pattern, size_t patternSize, uintptr_t *outAddress);

#endif // PROCESS_H_