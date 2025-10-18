#ifndef MEMORY_H_
#define MEMORY_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct ProcessHandle ProcessHandle;

ProcessHandle *memoryOpenProcess(const char *executableName);
void memoryWaitUntilProcessCloses(ProcessHandle *ph);
void memoryCloseProcess(ProcessHandle *ph);

bool memoryRead(ProcessHandle *ph, uint32_t address, void *buffer, size_t size);
bool memoryWrite(ProcessHandle *ph, uint32_t address, const void *buffer, size_t size);

bool memoryAllocatePage(ProcessHandle *ph, size_t size, uintptr_t *address);
bool memoryVirtualProtect(ProcessHandle *ph, uint32_t address, size_t size, uint32_t protect, uint32_t *oldProtect);

#endif // MEMORY_H_