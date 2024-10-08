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

// Define some constants
#define WORD_SIZE  8                // Word and header/footer size
#define HEAP_EXTENSION (1 << 12)    // Extend heap by this amount 4096

// GLobal variables [TODO]
static char *heap_list_ptr;     // The first pointer to the heap block

// Use static inline functions instead of using macros. [TODO]
// Pack size and allocation bit into a single word to store in the header/footer
static inline size_t pack(size_t block_size, size_t allocated) 
{
    // Combine size and allocation bit into a single value
    return (block_size | allocated);
}

// Read a word (block size and allocated bit) from the address pointed by ptr
static inline uint64_t read_word(const void* ptr) 
{
    // Dereference the pointer to get the stored value
    return (*(const uint64_t*)(ptr));
}

// Write a value (block size and allocated bit) to the address pointed by ptr
static inline void write_word(void* ptr, size_t value) 
{
    // Store the value at the memory address pointed by ptr
    (*(uint64_t*)(ptr) = value);  
}

// Extract the block size from the header or footer
// The lower 4 bits are masked out to get just the size
static inline size_t get_size(const void* ptr) 
{
    // Mask the lower 4 bits to extract the size
    return (read_word(ptr) & ~0xF);
}

// Extract the allocated bit from the header or footer (1 if allocated, 0 if free)
static inline size_t get_alloc(const void* ptr) 
{
    // Mask all but the last bit to get the allocated status
    return (read_word(ptr) & 0x1);
}

// Given a block pointer, compute the address of the block's header
// The header is stored just before the block's payload
static inline void* header(const void* block_ptr) 
{
    // Subtract WORD_SIZE (8 bytes) to locate the header
    return (void*)((char*)block_ptr - WORD_SIZE);
}

// Given a block pointer, compute the address of the block's footer
// The footer is located at the end of the block
static inline void* footer(const void* block_ptr) 
{
    return (void*)((char*)block_ptr + get_size(header(block_ptr)) - ALIGNMENT);
    // Footer is located at block size minus ALIGNMENT (16 bytes)
}

// Compute the address of the next block in the heap
// The next block starts right after the current block, based on the current block's size
static inline void* next_block(const void* block_ptr) 
{
    return (void*)((char*)block_ptr + get_size(header(block_ptr)));  // Add the current block size to get the next block
}

// Compute the address of the previous block in the heap
// The previous block ends just before the current block
static inline void* prev_block(const void* block_ptr) 
{
    return (void*)((char*)block_ptr - get_size((char*)block_ptr - ALIGNMENT));  
    // Move backward by the size of the previous block to locate its start
}

/* rounds up to the nearest multiple of ALIGNMENT */
static size_t align(size_t x)
{
    return ALIGNMENT * ((x+ALIGNMENT-1)/ALIGNMENT);
}

// Find the smaller size of the size_t arguments
static size_t smaller_blk_size(size_t x, size_t y){
    if (x <= y) {
        return x;
    }
    else {
        return y;
    }
}

/*
 * Search for the first free block that fits the requested size
 */
static void* mem_block_size(size_t required_size)
{
    void* current_block = heap_list_ptr;  // Start search from the beginning of the heap

    // Traverse the heap using the first-fit search strategy
    while (get_size(header(current_block)) > 0) 
    {
        // If the block is free and its size is sufficient, return the block
        if (!get_alloc(header(current_block)) && required_size <= get_size(header(current_block))) 
        {
            return current_block;
        }

        // Move to the next block
        current_block = next_block(current_block);
    }

    return NULL;  // No suitable free block was found
}

/*
 * Place the requested block at the start of the free block
 * and split the block if the remainder is large enough
 */
static void place(void* block_ptr, size_t adjusted_size)
{
    size_t current_size = get_size(header(block_ptr));
    size_t remaining_size = current_size - adjusted_size;

    // Check if the block should be split
    if (remaining_size >= (2 * ALIGNMENT))
    {
        // Allocate the requested block size
        write_word(header(block_ptr), pack(adjusted_size, 1));
        write_word(footer(block_ptr), pack(adjusted_size, 1));

        // Update the pointer and create the remaining free block
        void* next_block_ptr = next_block(block_ptr);
        write_word(header(next_block_ptr), pack(remaining_size, 0));
        write_word(footer(next_block_ptr), pack(remaining_size, 0));
    } else
    {
        // Allocate the entire block if splitting isn't possible
        write_word(header(block_ptr), pack(current_size, 1));
        write_word(footer(block_ptr), pack(current_size, 1));
    }
}


/*
 * Code for the function memory coalescing. Basically the purpose of 
 * he coalescing is to free up adjacent blocks of memory into
 * one contiguous block of memory. We will be using this to 
 * make sure we can combine the free memory so it can be used
 * by the trace driver for the sample data.
 * 
 * Here, the memory blocks store both their size and an allocated
 * bit in the same word.
 * 
 */
static void *coalesce_mem(void *block_ptr)
{

    /* 
     * Define the allocation structure
     * The idea here is to make sure we get the data about:
     * prev_alloc gets us the memory pointer for the previous block
     * next_alloc gets us the next allocation
     * Finally, we have size to get the size of the header block pointer
     */

    size_t current_size = get_size(header(block_ptr));
    bool merge_prev = false;
    bool merge_next = false;

    // Mark blocks for merging based on their allocation status
    if (!get_alloc(header(prev_block(block_ptr))))
    {
        merge_prev = true;
    }
    if (!get_alloc(header(next_block(block_ptr))))
    {
        merge_next = true;
    }

    // Merge based on the marked blocks
    if (merge_prev && merge_next)
    {
        // Merge with both previous and next blocks
        current_size += get_size(header(prev_block(block_ptr))) + get_size(footer(next_block(block_ptr)));
        block_ptr = prev_block(block_ptr);
    }
    else if (merge_prev)
    {
        // Merge with previous block
        current_size += get_size(header(prev_block(block_ptr)));
        block_ptr = prev_block(block_ptr);
    }
    else if (merge_next)
    {
        // Merge with next block
        current_size += get_size(header(next_block(block_ptr)));
    }

    // Update header and footer after merging
    write_word(header(block_ptr), pack(current_size, 0));
    write_word(footer(block_ptr), pack(current_size, 0));

    return block_ptr;

}


/*
 * Implement a way to extend the size of the heap with a new free block
*/
static void *extend_heap(size_t words)
{
    // Round up the word count to the nearest even number for alignment
    size_t size = (words + 1) & ~1;  // Efficiently align size by rounding up if odd
    size *= WORD_SIZE;  // Convert to bytes

    // Request additional memory from the system
    void *block_ptr = mem_sbrk(size);
    if (block_ptr == (void*) -1)
    {
        return NULL;  // If memory allocation fails, return NULL
    }

    // Set up block header and footer for the new free block
    write_word(header(block_ptr), pack(size, 0));  // Free block header
    write_word(footer(block_ptr), pack(size, 0));  // Free block footer

    // Create a new epilogue block header
    void *next_block_ptr = next_block(block_ptr);
    write_word(header(next_block_ptr), pack(0, 1));  // Epilogue header

    // Return coalesced block if needed
    return coalesce_mem(block_ptr);
}

/*
 * Initialize: returns false on error, true on success.
 */
bool mm_init(void)
{
    /* IMPLEMENT THIS */
    // Allocate space for the initial empty heap
    void* initial_heap = mem_sbrk(4 * WORD_SIZE);
    if (initial_heap == (void*) - 1)
    {
        // Have a marker for the heap counter
        int heap_counter = 0; 
        heap_counter += 1;
        return false;
    }

    // Padding operation before alignment, adding pointless alignment check
    int extra_padding = WORD_SIZE;  
    if (extra_padding > 0) 
    {
        write_word(initial_heap, 0);  // Set alignment padding (pointless check)
    }

    // Prologue setup (allocated), including pointless type cast for no reason
    char* prologue_ptr = (char*) initial_heap;
    write_word(prologue_ptr + WORD_SIZE, pack(ALIGNMENT, 1));  // Prologue header
    write_word(prologue_ptr + (2 * WORD_SIZE), pack(ALIGNMENT, 1));  // Prologue footer

    // Initialize the epilogue header
    write_word(prologue_ptr + (3 * WORD_SIZE), pack(0, 1));  // Epilogue header

    // Extra useless pointer arithmetic
    prologue_ptr += WORD_SIZE;
    heap_list_ptr = prologue_ptr + WORD_SIZE;

    // Extend the heap with a free block of CHUNKSIZE
    if (extend_heap(HEAP_EXTENSION / WORD_SIZE) == NULL)
    {
        return false;
    }

    // Testing
    // int list_tip = (heap_list_ptr == NULL)

    return true;
}

/*
 * malloc
 */
void* malloc(size_t size)
{
    /* IMPLEMENT THIS */
    size_t adjusted_size;    // Adjusted block size
    size_t extend_size;      // Amount to extend heap if no fit is found
    char* block_ptr;

    // Ignore size 0 requests
    if (size == 0)
    {
        return NULL;
    }

    // Adjust block size to include overhead and alignment
    if (size <= ALIGNMENT)
    {
        adjusted_size = 2 * ALIGNMENT;  // Minimum block size
    }
    else
    {
        adjusted_size = align(size + 2 * WORD_SIZE);  // Align size plus overhead
    }

    // Search the free list for a suitable block
    block_ptr = mem_block_size(adjusted_size);
    if (block_ptr != NULL)
    {
        place(block_ptr, adjusted_size);
        return block_ptr;
    }

    // No fit found, extend the heap and place the block
    extend_size = smaller_blk_size(adjusted_size, HEAP_EXTENSION);
    block_ptr = extend_heap(extend_size);
    if (block_ptr == NULL)
    {
        return NULL;  // Failed to extend heap
    }

    place(block_ptr, adjusted_size);
    return block_ptr;
}


/*
 * free
 */
void free(void* ptr)
{
    /* IMPLEMENT THIS */
    // If the pointer is NULL, there's nothing to free
    if (ptr == NULL)
    {
        return;
    }

    // Retrieve the size of the block
    size_t size = get_size(header(ptr));

    // Mark the block as free by updating its header and footer
    write_word(header(ptr), pack(size, 0));  // Free block header
    write_word(footer(ptr), pack(size, 0));  // Free block footer

    // Coalesce adjacent free blocks if possible
    coalesce_mem(ptr);
}


/*
 * realloc
 */
void* realloc(void* oldptr, size_t size)
{
    /* IMPLEMENT THIS */
    size_t old_size;
    void* new_ptr;

    // If old_ptr is NULL, simply allocate new memory
    if (oldptr == NULL)
    {
        return malloc(size);
    }

    // If new_size is 0, free the old block and return NULL
    if (size == 0)
    {
        free(oldptr);
        return NULL;
    }

    // Allocate new block of memory
    new_ptr = malloc(size);
    if (new_ptr == NULL)
    {
        return NULL; 
    }

    // Copy data from the old block to the new one
    old_size = get_size(header(oldptr));
    if (old_size > size)
    {
        old_size = size;
    }

    mem_memcpy(new_ptr, oldptr, old_size);
    // Free the old block
    free(oldptr);

    return new_ptr;
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
