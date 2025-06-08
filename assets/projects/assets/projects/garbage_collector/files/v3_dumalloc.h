#ifndef DUMALLOC_H
#define DUMALLOC_H

#define FIRST_FIT 0
#define BEST_FIT 1

void duManagedInitMalloc(int searchType);
void** duManagedMalloc(int size);
void duManagedFree(void** mptr);
void duMemoryDump();
void minorCollection();

#define Managed(p) (*p)
#define Managed_t(t) t*

#endif