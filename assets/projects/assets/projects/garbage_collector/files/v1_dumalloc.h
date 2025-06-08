#ifndef V1_DUMALLOC_H
#define V1_DUMALLOC_H

#include <stddef.h>

#define FIRST_FIT 0
#define BEST_FIT 1

void duInitMalloc(int size, int strategy);
void* duMalloc(size_t size);
void duFree(void* ptr);
void duMemoryDump();

#endif
