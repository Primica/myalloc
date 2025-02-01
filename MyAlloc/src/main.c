#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h> 


/**
 * @file main.c
 * @brief This file contains macros for memory alignment.
 *
 * @details
 * - ALIGNMENT: Defines the alignment boundary.
 * - ALIGN(size): Aligns the given size to the nearest multiple of ALIGNMENT.
 *
 * The ALIGN macro ensures that the size is rounded up to the nearest multiple of the defined ALIGNMENT.
 * This is useful for memory allocation where specific alignment is required.
 */
#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))

/**
 * @struct heapchunk_t
 * @brief Represents a chunk of memory in a heap.
 *
 * This structure is used to manage chunks of memory in a heap. Each chunk
 * contains information about its size, whether it is currently in use, and
 * a pointer to the next chunk in the heap.
 *
 * @var heapchunk_t::size
 * Size of the memory chunk in bytes.
 *
 * @var heapchunk_t::inuse
 * Flag indicating whether the chunk is currently in use (1) or free (0).
 *
 * @var heapchunk_t::next
 * Pointer to the next chunk in the heap.
 */
struct heapchunk_t {
    uint32_t size;
    uint8_t inuse;
    struct heapchunk_t *next;
};

/**
 * @struct heapinfo_t
 * @brief Represents information about a heap.
 *
 * This structure holds information about the heap, including a pointer to the
 * start of the heap chunks and the available memory.
 *
 * @var heapinfo_t::start
 * Pointer to the start of the heap chunks.
 *
 * @var heapinfo_t::avail
 * Available memory in the heap.
 */
struct heapinfo_t {
    struct heapchunk_t *start;
    uint32_t avail;
};

/**
 * Allocates a block of memory from the heap.
 *
 * This function searches for a free chunk of memory in the heap that is large enough to satisfy the requested size.
 * If a suitable chunk is found, it is marked as in use, and if the chunk is significantly larger than the requested size,
 * it is split into two chunks. The first chunk is returned to the caller, and the second chunk remains in the heap as free space.
 *
 * @param heap A pointer to the heap information structure.
 * @param size The size of the memory block to allocate, in bytes.
 * @return A pointer to the allocated memory block, or NULL if no suitable chunk is found.
 */
void *heap_alloc(struct heapinfo_t *heap, uint32_t size) {
    size = ALIGN(size);
    struct heapchunk_t *chunk = heap->start;
    while (chunk != NULL) {
        if (!chunk->inuse && chunk->size >= size) {
            chunk->inuse = true;
            if (chunk->size >= size + sizeof(struct heapchunk_t) + ALIGNMENT) {
                struct heapchunk_t *new_chunk = (struct heapchunk_t *)((uint8_t *)chunk + sizeof(struct heapchunk_t) + size);
                new_chunk->size = chunk->size - size - sizeof(struct heapchunk_t);
                new_chunk->inuse = false;
                new_chunk->next = chunk->next;
                chunk->next = new_chunk;
                chunk->size = size;
            }
            return (void *)(chunk + 1);
        }
        chunk = chunk->next;
    }
    return NULL; // No suitable chunk found
}

/**
 * @brief Reallocates a memory block with a new size.
 *
 * This function attempts to resize the memory block pointed to by `ptr` to 
 * `size` bytes. If `ptr` is NULL, it behaves like `malloc(size)`. If `size` 
 * is 0, it behaves like `free(ptr)` and returns NULL.
 *
 * @param ptr Pointer to the memory block to be reallocated. If NULL, a new 
 *            memory block is allocated.
 * @param size The new size of the memory block in bytes. If 0, the memory 
 *             block is freed.
 * @return A pointer to the newly allocated memory block, or NULL if the 
 *         allocation fails or if `size` is 0.
 */
void *heap_realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return malloc(size);
   }
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    size = ALIGN(size);
    struct heapchunk_t *chunk = (struct heapchunk_t *)ptr - 1;
    if (chunk->size >= size) {
        return ptr;
    }
    void *new_ptr = malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }

    memcpy(new_ptr, ptr, chunk->size);
    free(ptr);
    return new_ptr;
}

/**
 * @brief Allocates memory for an array of nmemb elements of size bytes each and initializes all bytes in the allocated storage to zero.
 *
 * This function is a custom implementation of calloc. It allocates memory for an array of nmemb elements, each of size bytes,
 * and initializes all bytes in the allocated memory to zero.
 *
 * @param nmemb Number of elements to allocate.
 * @param size Size of each element.
 * @return Pointer to the allocated memory, or NULL if the allocation fails.
 */
void *heap_calloc(size_t nmemb, size_t size) {
    size_t total_size = nmemb * size;
    void *ptr = malloc(total_size);
    if (ptr == NULL) {
        return NULL;
    }
    memset(ptr, 0, total_size);
    return ptr;
}

/**
 * Frees a previously allocated chunk of memory in the heap.
 *
 * @param heap A pointer to the heap information structure.
 * @param ptr A pointer to the memory chunk to be freed. If NULL, the function does nothing.
 *
 * This function marks the specified memory chunk as free. It does coalesce
 * adjacent free chunks. The available memory in the heap is updated accordingly.
 */
void heap_free(struct heapinfo_t *heap, void *ptr) {
    if (ptr == NULL) {
        return;
    }
    struct heapchunk_t *chunk = (struct heapchunk_t *)ptr - 1;
    chunk->inuse = false;

    struct heapchunk_t *current = heap->start;
    while (current != NULL) {
        if (!current->inuse && current->next != NULL && !current->next->inuse) {
            current->size += sizeof(struct heapchunk_t) + current->next->size;
            current->next = current->next->next;
        }
        current = current->next;
    }

    heap->avail = 0;
    current = heap->start;
    while (current != NULL) {
        if (!current->inuse) {
            heap->avail += current->size;
        }
        current = current->next;
    }
}

/**
 * @brief Initializes a heap with the given start address and size.
 *
 * This function sets up the initial heap structure by assigning the start address,
 * setting the size of the first chunk, marking it as not in use, and initializing
 * the available memory size.
 *
 * @param heap Pointer to the heapinfo_t structure to be initialized.
 * @param start Pointer to the start address of the heap memory.
 * @param size Total size of the heap memory in bytes.
 */
void heap_init(struct heapinfo_t *heap, void *start, const uint32_t size) {
    heap->start = (struct heapchunk_t *)start;
    heap->start->size = size - sizeof(struct heapchunk_t);
    heap->start->inuse = false;
    heap->start->next = NULL;
    heap->avail = size - sizeof(struct heapchunk_t);
}

/**
 * @brief Returns the size of a previously allocated memory block.
 *
 * This function retrieves the size of a memory block that was previously allocated
 * from the heap. The size is stored in the heapchunk_t structure that precedes the
 * memory block.
 *
 * @param ptr A pointer to the allocated memory block.
 * @return The size of the allocated memory block in bytes, or 0 if the pointer is NULL.
 */
uint32_t heap_sizeof(void *ptr) {
    if (ptr == NULL) {
        return 0;
    }
    struct heapchunk_t *chunk = (struct heapchunk_t *)ptr - 1;
    return chunk->size;
}

/**
 * @brief Prints information about the heap.
 *
 * This function prints information about the heap, including the start address,
 * the available memory, and details about each chunk in the heap.
 *
 * @param heap Pointer to the heapinfo_t structure containing information about the heap.
 */
char *heap_info(struct heapinfo_t *heap) {
    static char buffer[1024];
    char *ptr = buffer;
    ptr += sprintf(ptr, "Heap start: %p\n", heap->start);
    ptr += sprintf(ptr, "Available memory: %u bytes\n", heap->avail);
    struct heapchunk_t *chunk = heap->start;
    while (chunk != NULL) {
        ptr += sprintf(ptr, "Chunk: %p, size: %u, inuse: %d\n", chunk, chunk->size, chunk->inuse);
        chunk = chunk->next;
    }
    return buffer;
}

/**
 * @brief Main function to demonstrate the heap allocator.
 */
int main() {
    // Allocate a block of memory for the heap
    const uint32_t heap_size = 4096;
    void *heap_memory = mmap(NULL, heap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (heap_memory == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Initialize the heap
    struct heapinfo_t heap;
    heap_init(&heap, heap_memory, heap_size);

    // Allocate memory blocks from the heap
    void *ptr1 = heap_alloc(&heap, 128);
    void *ptr2 = heap_alloc(&heap, 256);
    void *ptr3 = heap_alloc(&heap, 512);

    // Print information about the heap
    printf("%s", heap_info(&heap));

    // Free the memory blocks
    heap_free(&heap, ptr1);
    heap_free(&heap, ptr2);
    heap_free(&heap, ptr3);

    // Print information about the heap
    printf("%s", heap_info(&heap));

    // Unmap the heap memory
    munmap(heap_memory, heap_size);

    return 0;
}
