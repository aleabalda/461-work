#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include "myalloc.h"

static void *_arena_start = NULL;
static size_t _arena_size = 0;

static node_t *firstNode = NULL;
static node_t *lastNode = NULL;

int statusno = 0;

int myinit(size_t size)
{
    // Check if size is within valid range
    if (size > MAX_ARENA_SIZE || size <= 0)
    {
        statusno = ERR_BAD_ARGUMENTS; // Set error status
        return ERR_BAD_ARGUMENTS;     // Return error code
    }

    // Check if the arena has already been initialized
    if (_arena_start != NULL)
    {
        statusno = ERR_CALL_FAILED; // Set error status
        return ERR_CALL_FAILED;     // Return error code
    }

    size_t page_size = getpagesize();                               // Get the system's page size
    _arena_size = ((size + page_size - 1) / page_size) * page_size; // Calculate the adjusted arena size to align with page boundaries

    // Allocate memory using mmap
    _arena_start = mmap(NULL, _arena_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    // Check if mmap failed
    if (_arena_start == MAP_FAILED)
    {
        _arena_start = NULL;
        statusno = ERR_SYSCALL_FAILED; // Set error status
        return ERR_SYSCALL_FAILED;     // Return error code
    }

    // Initialize the first node representing the entire allocated space
    firstNode = (node_t *)_arena_start;
    firstNode->size = _arena_size - sizeof(node_t);
    firstNode->is_free = 1;
    firstNode->fwd = NULL;
    firstNode->bwd = NULL;

    // Set lastNode to the firstNode since it's the only node in the linked list at this point
    lastNode = firstNode;

    return _arena_size; // Return the size of the allocated arena
}

int mydestroy()
{
    // Check if the arena has been initialized
    if (_arena_start == NULL)
    {
        statusno = ERR_UNINITIALIZED; // Set error status
        return ERR_UNINITIALIZED;     // Return error code
    }

    // De-allocate memory using munmap
    int map = munmap(_arena_start, _arena_size);

    // Check if munmap failed
    if (map == -1)
    {
        statusno = ERR_SYSCALL_FAILED; // Set error status
        return ERR_SYSCALL_FAILED;     // Return error code
    }

    // Reset all relevant variables and pointers
    _arena_start = NULL;
    _arena_size = 0;
    firstNode = NULL;
    lastNode = NULL;

    return 0; // Return success code
}

void *myalloc(size_t size)
{
    // Check if the arena has been initialized
    if (_arena_start == NULL)
    {
        statusno = ERR_UNINITIALIZED; // Set error status
        return NULL;                  // Return NULL to indicate failure
    }

    node_t *current = firstNode; // Initialize a pointer to traverse the linked list of blocks

    // Iterate through the linked list
    while (current != NULL)
    {
        // Check if the current block is free and large enough for allocation
        if (current->is_free && current->size >= size)
        {
            size_t totalsize = size + sizeof(node_t); // Calculate the total size required for allocation (including header)

            // Check if the current block can be split into two blocks
            if (current->size > size + sizeof(node_t))
            {
                // Create a new node for the remaining free space
                node_t *newNode = (node_t *)((void *)current + totalsize);
                newNode->size = current->size - totalsize;
                newNode->is_free = 1;
                newNode->fwd = current->fwd;
                newNode->bwd = current;

                // Update the current block with the allocated size and mark it as not free
                current->size = size;
                current->is_free = 0;
                current->fwd = newNode;

                // Update forward pointer of the next block, if it exists
                if (newNode->fwd != NULL)
                {
                    newNode->fwd->bwd = newNode;
                }
                else
                {
                    lastNode = newNode; // Update lastNode if newNode is the last block
                }
            }
            else
            {
                current->is_free = 0; // Mark the current block as not free
            }

            return (void *)current + sizeof(node_t); // Return the pointer to the allocated memory
        }

        current = current->fwd; // Move to the next block
    }

    statusno = ERR_OUT_OF_MEMORY; // Set error status for out of memory
    return NULL;                  // Return NULL to indicate failure
}

void myfree(void *ptr)
{
    // Check if the arena has been initialized and if the pointer is NULL
    if (_arena_start == NULL || ptr == NULL)
    {
        statusno = ERR_UNINITIALIZED; // Set error status
        return;                       // Exit the function
    }

    node_t *block = (node_t *)((char *)ptr - sizeof(node_t)); // Calculate the header of the allocated block from the pointer provided by the user
    block->is_free = 1;                                       // Mark the block as free

    // Coalesce with forward free blocks
    if (block->fwd && block->fwd->is_free)
    {
        block->size += block->fwd->size + sizeof(node_t); // Increase the size of the current block to include the size of the forward block and header
        block->fwd = block->fwd->fwd;                     // Skip over the forward block by updating the forward pointer

        if (block->fwd)
        {
            block->fwd->bwd = block; // Update the backward pointer of the new forward block
        }
    }

    // Coalesce with backward free block
    if (block->bwd && block->bwd->is_free)
    {
        block->bwd->size += block->size + sizeof(node_t);
        block->bwd->fwd = block->fwd;

        if (block->fwd)
        {
            block->fwd->bwd = block->bwd;
        }
    }

    statusno = 0; // Reset status to indicate successful operation
}
