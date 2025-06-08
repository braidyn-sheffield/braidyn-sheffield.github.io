#include <stdio.h>
#include <string.h>
#include "dumalloc.h"

#define HEAP_SIZE 128*8 // 1024 bytes
#define FIRST_FIT 0
#define BEST_FIT 1
#define MAX_MANAGED 128

#define HEAP_COUNT 2 // Number of heaps

int allocationStrategy = FIRST_FIT; // default

unsigned char heap[HEAP_COUNT][HEAP_SIZE]; // The heap is a static array of bytes
int currentHeap = 0; // Current heap index
void *managedList[MAX_MANAGED]; // Array to keep track of managed pointers
int managedListSize = 0;

typedef struct memoryBlockHeader {
    int free;           // 0 = used, 1 = free
    int size;           // size of the user data
    int managedIndex;   // index into the ManagedList
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

    for (int i = 0; i < HEAP_SIZE; i++)
    {
        heap[toHeap][i] = 0; // Zeroing out the second heap
    }

    unsigned char* destPtr = heap[toHeap];

    for (int i = 0; i < managedListSize; i++)
    {
        if (managedList[i] != 0)
        {
            memoryBlockHeader* oldHeader = (memoryBlockHeader*)((unsigned char*)managedList[i] - sizeof(memoryBlockHeader)); // Get the header of the old block
            int totalSize = oldHeader->size + sizeof(memoryBlockHeader); // Total size of the old block

            memcpy(destPtr, oldHeader, totalSize); // Copy the old block to the new heap

            memoryBlockHeader* newHeader = (memoryBlockHeader*)destPtr; // Get the header of the new block
            newHeader->free = 0; // Mark the new block as used
            newHeader->next = 0; // Set the next pointer to null

            managedList[i] = destPtr + sizeof(memoryBlockHeader); // Update the managed list with the new pointer

            destPtr += totalSize; // Move the destination pointer forward
        }
    }

    int remainingSize = heap[toHeap] + HEAP_SIZE - destPtr; // Calculate the remaining size in the new heap

    if (remainingSize > sizeof(memoryBlockHeader))
    {
        memoryBlockHeader* newFree = (memoryBlockHeader*)destPtr; // Create a new free block
        newFree->size = remainingSize - sizeof(memoryBlockHeader); // Set the size of the free block
        newFree->next = 0; // Set the next pointer to null
        newFree->free = 1; // Mark the block as free

        freeListHead[toHeap] = newFree; // Set the free list head to the new free block
    }
    else
    {
        freeListHead[toHeap] = 0; // No free block available
    }

    currentHeap = toHeap; // Switch to the new heap
}