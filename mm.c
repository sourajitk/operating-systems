/*
 * mm.c
 *
 * Name 1: Sourajit Karmakar
 * PSU ID 1: 921859793
 *
 * Name 2: Ann Zezyus
 * PSU ID 2: 990191582
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 * Also, read malloclab.pdf carefully and in its entirety before beginning.
 *
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include "mm.h"
#include "memlib.h"

/*
 * If you want to enable your debugging output and heap checker code,
 * uncomment the following line. Be sure not to have debugging enabled
 * in your final submission.
 * 
 * We are implementing memory allocator functions that are typically provided by
 * the standard C library.
 * 
 */
// #define DEBUG

#ifdef DEBUG
/* When debugging is enabled, the underlying functions get called */
#define dbg_printf(...) printf(__VA_ARGS__)
#define dbg_assert(...) assert(__VA_ARGS__)
#else
/* When debugging is disabled, no code gets generated */
#define dbg_printf(...)
#define dbg_assert(...)
#endif /* DEBUG */

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#define memset mem_memset
#define memcpy mem_memcpy
#endif /* DRIVER */


/* 
 * Number of blocks to be aligned on a 16-byte boundary.
 * This is then later used with align() to round to the nearest
 * multiple of 16.
 */
#define ALIGNMENT 16

// GLobal variables [TODO]
static int is_mm_inited = 0;
struct node_t {
    size_t size; // The size of the memory block
};

// Use static inline functions instead of using macros. [TODO]

/* rounds up to the nearest multiple of ALIGNMENT */
static size_t align(size_t x)
{
    return ALIGNMENT * ((x+ALIGNMENT-1)/ALIGNMENT);
}

/*
 * Initialize: returns false on error, true on success.
 */
bool mm_init(void)
{
    /* IMPLEMENT THIS */
    // Trying out a 311 style "mount_check" (might need more than just this)
    if (is_mm_inited == 0) {
        return false;
    } else {
        is_mm_inited = 1;
        return true;
    }
}

/*
 * 
 */
void* malloc(size_t size)
{
    /* IMPLEMENT THIS */
    // Align the requested size
    const size_t size_mm = align(size);
    
    // Define the node struct to manage memory blocks
    struct node_t *node;
    
    // Check if the aligned size is valid (greater than 0)
    if (size_mm == 0) {
        return NULL;
    }

    // Search for a suitable free block (implementation dependent)
    node = find_free_block(size_mm);

    if (node != NULL) {
        // Suitable free block found, remove it from the free list and allocate it
        remove_from_free_list(node);
        node->size = size_mm;
    } else {
        // No suitable block found, extend the heap
        node = extend_heap(size_mm);
        if (node == NULL) {
            return NULL;
        }
        node->size = size_mm;
    }

    // Return the pointer to the allocated memory (after the metadata/header)
    return (void*)(node + 1);

}

/*
 * free
 */
void free(void* ptr)
{
    /* IMPLEMENT THIS */
    return;
}

/*
 * realloc
 */
void* realloc(void* oldptr, size_t size)
{
    /* IMPLEMENT THIS */
    if (oldptr == NULL) {
        return malloc(size);
    }
    // Construct a buf node
    struct node_t* buf_ptr = (struct node_t*)oldptr - 1;

    // Check if size equals 0
    if (size == 0) {
        free(oldptr);
        return NULL;
    }

    // Check if the size matches
    if (buf_ptr->size >= size) {
        return oldptr;
    } else {
        // Allocate a new block of the input size and copy data over
        struct node_t* n_ptr = malloc(size);
        if (n_ptr == NULL) {
            return NULL;
        } else {
            memcpy(n_ptr, oldptr, buf_ptr->size);
            free(oldptr);
        }
        return n_ptr;
    }

    return NULL;
}

/*
 * calloc
 * This function is not tested by mdriver, and has been implemented for you.
 */
void* calloc(size_t nmemb, size_t size)
{
    void* ptr;
    size *= nmemb;
    ptr = malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

/*
 * Returns whether the pointer is in the heap.
 * May be useful for debugging.
 */
static bool in_heap(const void* p)
{
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Returns whether the pointer is aligned.
 * May be useful for debugging.
 */
static bool aligned(const void* p)
{
    size_t ip = (size_t) p;
    return align(ip) == ip;
}

/*
 * mm_checkheap
 */
bool mm_checkheap(int lineno)
{
#ifdef DEBUG
    /* Write code to check heap invariants here */
    /* IMPLEMENT THIS */

#endif /* DEBUG */
    return true;
}
