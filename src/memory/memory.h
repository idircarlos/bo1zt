#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct ProcessHandle ProcessHandle;

ProcessHandle *memoryOpenProcess(const char *executableName);
void memoryCloseProcess(ProcessHandle *ph);

bool memoryRead(ProcessHandle *ph, uint32_t address, void *buffer, size_t size);
bool memoryWrite(ProcessHandle *ph, uint32_t address, const void *buffer, size_t size);