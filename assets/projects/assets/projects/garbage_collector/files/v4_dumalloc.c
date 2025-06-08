#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dumalloc.h"

#define HEAP_SIZE 128*8 // 1024 bytes
#define FIRST_FIT 0
#define BEST_FIT 1
#define MAX_MANAGED 128

#define HEAP_COUNT 3 // Number of heaps
#define SURVIVAL_COUNT 3 // Number of minor collections before promotion

int allocationStrategy = FIRST_FIT; // default

unsigned char heap[HEAP_COUNT][HEAP_SIZE]; // The heap is a static array of bytes
int currentHeap = 0; // Current heap index
void *managedList[MAX_MANAGED]; // Array to keep track of managed pointers
int managedListSize = 0;

typedef struct memoryBlockHeader {
    int free;           // 0 = used, 1 = free
    int size;           // size of the user data
    int managedIndex;   // index into the ManagedList
    int survivalCount; // number of minor collections survived
    struct memoryBlockHeader* next;
} memoryBlockHeader;


memoryBlockHeader* freeListHead[HEAP_COUNT]; // Pointer to the head of the free list

void duManagedInitMalloc(int searchType);
void** duManagedMalloc(int size);
void duManagedFree(void** mptr);
void duMemoryDump(void);

void duInitMalloc(int searchType);
void* duMalloc(int size);
void duFree(void* ptr);

void printAllBlocks(int currentHeap);
void printHeapGraphic(int currentHeap);
void printFreeList(int currentHeap);
void printManagedList();

void minorCollection();
void majorCollection();
void* duMallocOnHeap(int size, int heapIndex);

void duManagedInitMalloc(int searchType)
{
    duInitMalloc(searchType); // Initialize the heap

    for (int i = 0; i < MAX_MANAGED; i++)
    {
        managedList[i] = 0; // Initialize the managed list
    }

}
void** duManagedMalloc(int size)
{
    void* ptr = duMalloc(size); // Allocate memory using the standard malloc

    if (ptr == 0)
    {
        return 0; // Allocation failed
    }

    managedList[managedListSize] = ptr; // Store the pointer in the managed list
    memoryBlockHeader* header = (memoryBlockHeader*)((unsigned char*)ptr - sizeof(memoryBlockHeader)); // Get the header of the allocated block
    header->managedIndex = managedListSize; // Store the index in the header

    void** managedPtr = &managedList[managedListSize]; // Create a pointer to the managed list entry

    managedListSize++;

    return managedPtr; // Return the pointer to the managed list entry


}
void duManagedFree(void** mptr)
{
    duFree(*mptr); // Free the memory using the standard free
    *mptr = 0; // Set the pointer to null


}

void printAllBlocks(int currentHeap)
{
    memoryBlockHeader* current = (memoryBlockHeader*)heap[currentHeap]; // Points to the first block in the heap

    while ((unsigned char*)current < heap[currentHeap] + HEAP_SIZE)
    {
        int offset = (unsigned char*)current - (unsigned char*)heap[currentHeap]; // Calculate the offset of the current block in the heap

        if (current->free == 0)
        {
            printf("Used at ");
        }
        else
        {
            printf("Free at ");
        }

        printf("%p (offset: %d), size: %d\n", (void*)current, offset, current->size);

        current = (memoryBlockHeader*)((unsigned char*)current + sizeof(memoryBlockHeader) + current->size);
    }
}

void printHeapGraphic(int currentHeap)
{
    memoryBlockHeader* current = (memoryBlockHeader*)heap[currentHeap]; // Points to the first block in the heap

    char cstring[129]; 
    cstring[128] = '\0';
    for (int i = 0; i < 128; i++)
    {
        cstring[i] = '_';
    }

    char currentFreeLetter = 'a';
    char currentUsedLetter = 'A';

    while ((unsigned char*)current < heap[currentHeap] + HEAP_SIZE)
    {
        int chunkSize = (sizeof(memoryBlockHeader) + current->size) / 8;

        int chunkIndex = ((unsigned char*)current - heap[currentHeap]) / 8;

        if (current->free == 0)
        {
            for (int i = 0; i < chunkSize; i++)
            {
                    cstring[chunkIndex + i] = currentUsedLetter;
            }

            currentUsedLetter++;
        }
        else
        {
            for (int i = 0; i < chunkSize; i++)
            {
                cstring[chunkIndex + i] = currentFreeLetter;
            }

            currentFreeLetter++;
        }
        
        
        current = (memoryBlockHeader*)((unsigned char*)current + sizeof(memoryBlockHeader) + current->size);
    }

    printf("%s\n",cstring);
}

void printFreeList(int currentHeap)
{
    memoryBlockHeader* current = freeListHead[currentHeap]; // Start from the head of the free list

    while (current != 0) // Traverse the free list
    {
        int offset = (unsigned char*)current - (unsigned char*)heap[currentHeap]; // Calculate the offset of the current block in the heap

        printf("Block at %p (offset: %d), size %d\n", (void*)current, offset, current->size); // Print the address, offset, and size of the current block

        current = current->next; // Move to the next block in the free list
    }
}

void printManagedList()
{
    printf("ManagedList\n");
    for (int i = 0; i < managedListSize; i++)
    {
        printf("ManagedList[%d] = %p\n", i, managedList[i]);
    }
}

void duInitMalloc(int searchType)
{
    allocationStrategy = searchType;

    for (int i =0; i < HEAP_SIZE; i++) // Zeroing out the heap
    {
        heap[currentHeap][i] = 0;
    }

    memoryBlockHeader* currentBlock = (memoryBlockHeader*)heap[currentHeap]; // The first block is at the start of the heap

    currentBlock->size = HEAP_SIZE - sizeof(memoryBlockHeader); // The size of the first block is the total heap size minus the header size
    currentBlock->next = 0; // The next pointer is null since this is the only block

    freeListHead[currentHeap] = currentBlock; // Set the free list head to the first block

    for (int i = 0; i < HEAP_SIZE; i++)
    {
        heap[2][i] = 0; // Zeroing out the second heap
    }

    memoryBlockHeader* secondHeapBlock = (memoryBlockHeader*)heap[2]; // The first block is at the start of the second heap
    secondHeapBlock->size = HEAP_SIZE - sizeof(memoryBlockHeader); // The size of the first block is the total heap size minus the header size
    secondHeapBlock->next = 0; // The next pointer is null since this is the only block
    secondHeapBlock->free = 1; // Mark the block as free
    secondHeapBlock->managedIndex = -1; // Set the managed index to -1
    secondHeapBlock->survivalCount = 0; // Set the survival count to 0

    freeListHead[2] = secondHeapBlock; // Set the free list head to the first block of the second heap
}

void duMemoryDump()
{
    printf("MEMORY DUMP\n");
    printf("Current Heap: %d\n", currentHeap);
    printf("Memory Block\n");

    printAllBlocks(currentHeap);
    printHeapGraphic(currentHeap);

    printf("Free List\n");
    printFreeList(currentHeap);

    printManagedList();


}

void* duMalloc(int size)
{
    // Ensure size is a multiple of 8 (alignment requirement)
    if (size % 8 != 0)
    {
        size += 8 - (size % 8);
    }

    int blockSize = size + sizeof(memoryBlockHeader); // Total space needed (header + payload)

    // -------------------------
    // FIRST FIT Allocation
    // -------------------------
    if (allocationStrategy == FIRST_FIT)
    {
        memoryBlockHeader* current = freeListHead[currentHeap];
        memoryBlockHeader* prev = 0;

        // Traverse the free list and stop at the first block large enough
        while (current != 0)
        {
            if (current->size >= size)
            {
                void* userBlock = (unsigned char*)current + sizeof(memoryBlockHeader);
                current->free = 0; // Mark block as used

                // Exact fit — remove the block from the free list
                if (current->size == size)
                {
                    if (current == freeListHead[currentHeap])
                    {
                        freeListHead[currentHeap] = current->next;
                    }
                    else
                    {
                        prev->next = current->next;
                    }
                    return userBlock;
                }

                // Split the block: allocate first part, create a new free block after it
                memoryBlockHeader* newFree = (memoryBlockHeader*)((unsigned char*)current + blockSize);
                newFree->size = current->size - blockSize;
                newFree->next = current->next;
                newFree->free = 1;

                if (current == freeListHead[currentHeap])
                {
                    freeListHead[currentHeap] = newFree;
                }
                else
                {
                    prev->next = newFree;
                }

                current->size = size;
                return userBlock;
            }

            prev = current;
            current = current->next;
        }

        return 0; // No suitable block found
    }
    // -------------------------
    // BEST FIT Allocation
    // -------------------------
    else
    {
        memoryBlockHeader* current = freeListHead[currentHeap];
        memoryBlockHeader* best = 0;
        memoryBlockHeader* bestPrev = 0;
        memoryBlockHeader* prev = 0;

        // Traverse the entire list to find the smallest block that fits
        while (current != 0)
        {
            if (current->size >= size)
            {
                if (best == 0 || current->size < best->size)
                {
                    best = current;
                    bestPrev = prev;
                }
            }

            prev = current;
            current = current->next;
        }

        if (best == 0)
        {
            return 0; // No suitable block found
        }

        void* userBlock = (unsigned char*)best + sizeof(memoryBlockHeader);
        best->free = 0; // Mark block as used

        // Exact fit — remove the block from the free list
        if (best->size == size)
        {
            if (best == freeListHead[currentHeap])
            {
                freeListHead[currentHeap] = best->next;
            }
            else
            {
                bestPrev->next = best->next;
            }
            return userBlock;
        }

        // Split the block: allocate first part, create a new free block after it
        memoryBlockHeader* newFree = (memoryBlockHeader*)((unsigned char*)best + blockSize);
        newFree->size = best->size - blockSize;
        newFree->next = best->next;
        newFree->free = 1;

        if (best == freeListHead[currentHeap])
        {
            freeListHead[currentHeap] = newFree;
        }
        else
        {
            bestPrev->next = newFree;
        }

        best->size = size;
        return userBlock;
    }
}


void duFree(void* ptr)
{
    memoryBlockHeader* ptrHeader = (memoryBlockHeader*)((unsigned char*)ptr - sizeof(memoryBlockHeader)); // Get the header of the block to be freed

    ptrHeader->free = 1;

    memoryBlockHeader* current = freeListHead[currentHeap]; // Start from the head of the free list

    memoryBlockHeader* prev = 0; // Previous block pointer

    while (current != 0 && current < ptrHeader) // Traverse the free list to find the correct position for the freed block
    {
        prev = current;
        current = current->next;
    }

    ptrHeader->next = current; // Link the freed block to the next block in the free list

    if (prev == 0) // If the freed block is the head of the free block
    {
        freeListHead[currentHeap] = ptrHeader; // Move the head to the freed block
    }
    else
    {
        prev->next = ptrHeader; // Link the previous block to the freed block
    }

}

void minorCollection()
{
    int fromHeap = currentHeap;
    int toHeap = 1 - currentHeap;

    // Clear the toHeap before copying
    for (int i = 0; i < HEAP_SIZE; i++)
    {
        heap[toHeap][i] = 0;
    }

    unsigned char* destPtr = heap[toHeap];

    for (int i = 0; i < managedListSize; i++)
    {
        if (managedList[i] != NULL)
        {
            memoryBlockHeader* oldHeader = (memoryBlockHeader*)((unsigned char*)managedList[i] - sizeof(memoryBlockHeader));

            // Only relocate if the block is from the young generation
            if ((unsigned char*)oldHeader >= heap[fromHeap] && (unsigned char*)oldHeader < heap[fromHeap] + HEAP_SIZE)
            {
                oldHeader->survivalCount++;

                // 🚀 Promote to old generation if survival threshold is reached
                if (oldHeader->survivalCount >= SURVIVAL_COUNT)
                {
                    void* promoted = duMallocOnHeap(oldHeader->size, 2);
                    if (promoted == NULL)
                    {
                        printf("Promotion failed for managedList[%d]\n", i);
                        exit(1);
                    }

                    memoryBlockHeader* newHeader = (memoryBlockHeader*)((unsigned char*)promoted - sizeof(memoryBlockHeader));
                    newHeader->managedIndex = oldHeader->managedIndex;
                    newHeader->survivalCount = oldHeader->survivalCount;
                    memcpy(newHeader + 1, oldHeader + 1, oldHeader->size);

                    managedList[i] = (unsigned char*)newHeader + sizeof(memoryBlockHeader);
                }
                else
                {
                    // Copy to toHeap (young generation)
                    int totalSize = oldHeader->size + sizeof(memoryBlockHeader);
                    memcpy(destPtr, oldHeader, totalSize);

                    memoryBlockHeader* newHeader = (memoryBlockHeader*)destPtr;
                    newHeader->next = NULL;

                    managedList[i] = destPtr + sizeof(memoryBlockHeader);
                    destPtr += totalSize;
                }
            }
        }
    }

    // Add a free block if space remains in toHeap
    int remainingSize = (heap[toHeap] + HEAP_SIZE) - destPtr;

    if (remainingSize > sizeof(memoryBlockHeader))
    {
        memoryBlockHeader* newFree = (memoryBlockHeader*)destPtr;
        newFree->size = remainingSize - sizeof(memoryBlockHeader);
        newFree->next = NULL;
        newFree->free = 1;
        newFree->managedIndex = -1;
        newFree->survivalCount = 0;

        freeListHead[toHeap] = newFree;
    }
    else
    {
        freeListHead[toHeap] = NULL;
    }

    currentHeap = toHeap; // Switch to the new heap
}

void majorCollection() {
    int oldHeap = 2; // Old generation heap index
    unsigned char* heapStart = heap[oldHeap];
    unsigned char* heapEnd = heap[oldHeap] + HEAP_SIZE;

    memoryBlockHeader* src = (memoryBlockHeader*)heapStart;  // Scans the heap
    unsigned char* destPtr = heapStart;                      // Compaction destination

    // Reset free list for old heap
    freeListHead[oldHeap] = 0;

    while ((unsigned char*)src < heapEnd) {
        int totalSize = sizeof(memoryBlockHeader) + src->size;

        if (src->free == 0) {
            if ((unsigned char*)src != destPtr) {
                // Move block forward
                memcpy(destPtr, src, totalSize);

                memoryBlockHeader* newHeader = (memoryBlockHeader*)destPtr;

                // Update managed pointer to new location
                if (newHeader->managedIndex >= 0 && newHeader->managedIndex < MAX_MANAGED) {
                    managedList[newHeader->managedIndex] = (unsigned char*)newHeader + sizeof(memoryBlockHeader);
                }
            }
            destPtr += totalSize;
        }

        // Move to the next block
        src = (memoryBlockHeader*)((unsigned char*)src + totalSize);
    }

    // Add one large free block with the remaining space
    int remaining = heapEnd - destPtr;
    if (remaining >= (int)sizeof(memoryBlockHeader)) {
        memoryBlockHeader* freeBlock = (memoryBlockHeader*)destPtr;
        freeBlock->size = remaining - sizeof(memoryBlockHeader);
        freeBlock->next = 0;
        freeBlock->free = 1;
        freeBlock->managedIndex = -1;

        freeListHead[oldHeap] = freeBlock;
    } else {
        // No space left for even a free block
        freeListHead[oldHeap] = 0;
    }
}

void* duMallocOnHeap(int size, int heapIndex) 
{
    // Align size to 8 bytes
    if (size % 8 != 0) 
    {
        size += 8 - (size % 8);
    }

    // Validate heap index
    if (heapIndex < 0 || heapIndex >= HEAP_COUNT) 
    {
        return 0;
    }

    int blockSize = size + sizeof(memoryBlockHeader); // Total block size

    unsigned char* heapStart = heap[heapIndex];
    unsigned char* heapEnd = heap[heapIndex] + HEAP_SIZE;

    memoryBlockHeader* current = freeListHead[heapIndex]; // Search from the free list
    memoryBlockHeader* prev = 0;

    while ((unsigned char*)current < heapEnd && current != 0) 
    {
        if (current->free && current->size >= size) 
        {
            void* userBlock = (unsigned char*)current + sizeof(memoryBlockHeader);
            current->free = 0;

            // Check if we can split the block
            if (current->size - size >= (int)sizeof(memoryBlockHeader) + 8) 
            {
                memoryBlockHeader* newFree = (memoryBlockHeader*)((unsigned char*)current + blockSize);
                newFree->size = current->size - blockSize;
                newFree->next = current->next;
                newFree->free = 1;
                newFree->managedIndex = -1;
                newFree->survivalCount = 0;

                if (current == freeListHead[heapIndex]) 
                {
                    freeListHead[heapIndex] = newFree;
                } 
                else if (prev != 0) 
                {
                    prev->next = newFree;
                }

                current->size = size;
                current->next = 0; // It is now an allocated block
            } 
            else 
            {
                // Can't split, just remove from free list
                if (current == freeListHead[heapIndex]) 
                {
                    freeListHead[heapIndex] = current->next;
                } 
                else if (prev != 0) 
                {
                    prev->next = current->next;
                }
            }

            return userBlock;
        }

        prev = current;
        current = current->next;
    }

    return 0; // No suitable block found
}
