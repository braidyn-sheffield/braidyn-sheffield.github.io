#include <stdio.h>
#include "dumalloc.h"

#define HEAP_SIZE 128*8 // 1024 bytes
#define FIRST_FIT 0
#define BEST_FIT 1

int allocationStrategy = FIRST_FIT; // default

unsigned char heap[HEAP_SIZE]; // The heap is a static array of bytes

typedef struct memoryBlockHeader {
    int free; // 0 - used, 1 = free
    int size; // size of the reserved block
    struct memoryBlockHeader* next;  // the next block in the integrated free list
} memoryBlockHeader;

memoryBlockHeader* freeListHead; // Pointer to the head of the free list

void printAllBlocks()
{
    memoryBlockHeader* current = (memoryBlockHeader*)heap; // Points to the first block in the heap

    while ((unsigned char*)current < heap + HEAP_SIZE)
    {
        int offset = (unsigned char*)current - (unsigned char*)heap; // Calculate the offset of the current block in the heap

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

void printHeapGraphic()
{
    memoryBlockHeader* current = (memoryBlockHeader*)heap; // Points to the first block in the heap

    char cstring[129]; 
    cstring[128] = '\0';
    for (int i = 0; i < 128; i++)
    {
        cstring[i] = '_';
    }

    char currentFreeLetter = 'a';
    char currentUsedLetter = 'A';

    while ((unsigned char*)current < heap + HEAP_SIZE)
    {
        int chunkSize = (sizeof(memoryBlockHeader) + current->size) / 8;

        int chunkIndex = ((unsigned char*)current - heap) / 8;

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

void printFreeList()
{
    memoryBlockHeader* current = freeListHead; // Start from the head of the free list

    while (current != 0) // Traverse the free list
    {
        int offset = (unsigned char*)current - (unsigned char*)heap; // Calculate the offset of the current block in the heap

        printf("Block at %p (offset: %d), size %d\n", (void*)current, offset, current->size); // Print the address, offset, and size of the current block

        current = current->next; // Move to the next block in the free list
    }
}

void duInitMalloc(int searchType)
{
    allocationStrategy = searchType;

    for (int i =0; i < HEAP_SIZE; i++) // Zeroing out the heap
    {
        heap[i] = 0;
    }

    memoryBlockHeader* currentBlock = (memoryBlockHeader*)heap; // The first block is at the start of the heap

    currentBlock->size = HEAP_SIZE - sizeof(memoryBlockHeader); // The size of the first block is the total heap size minus the header size
    currentBlock->next = 0; // The next pointer is null since this is the only block

    freeListHead = currentBlock; // Set the free list head to the first block
}

void duMemoryDump()
{
    printf("MEMORY DUMP\n");
    printf("Memory Block\n");

    printAllBlocks();
    printHeapGraphic();

    printf("Free List\n");
    printFreeList();


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
        memoryBlockHeader* current = freeListHead;
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
                    if (current == freeListHead)
                    {
                        freeListHead = current->next;
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

                if (current == freeListHead)
                {
                    freeListHead = newFree;
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
        memoryBlockHeader* current = freeListHead;
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
            if (best == freeListHead)
            {
                freeListHead = best->next;
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

        if (best == freeListHead)
        {
            freeListHead = newFree;
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

    memoryBlockHeader* current = freeListHead; // Start from the head of the free list

    memoryBlockHeader* prev = 0; // Previous block pointer

    while (current != 0 && current < ptrHeader) // Traverse the free list to find the correct position for the freed block
    {
        prev = current;
        current = current->next;
    }

    ptrHeader->next = current; // Link the freed block to the next block in the free list

    if (prev == 0) // If the freed block is the head of the free block
    {
        freeListHead = ptrHeader; // Move the head to the freed block
    }
    else
    {
        prev->next = ptrHeader; // Link the previous block to the freed block
    }

}
