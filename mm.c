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
 * References:
 * https://danluu.com/malloc-tutorial/
 * https://www.tutorialspoint.com/c_standard_library/c_function_malloc.htm
 * https://manual.cs50.io/3/malloc
 * https://man7.org/linux/man-pages/man3/malloc.3.html
 * Randal E. Bryant and David R. O'Hallaron. 2015. Computer Systems: A Programmer's Perspective (3rd. ed.). Pearson. [Book]
 * 
 * 
 * Memory Management Strategy - Segregated Free Lists
 * The heap is organized into segregated free lists based on block sizes. 
 * Each block includes a header and footer that record the block's size and allocation status.
 * 
 * Memory Allocation - Using free_lists() to manage block allocation
 * We are useing segregated free lists to manage block allocation efficiently across
 * varying block sizes. When a malloc request is made, the allocator calculates the required
 * size and selects the appropriate size class, grouping similar sizes together to minimize
 * search time. It then searches the list for a suitable free block and, if found, allocates it.
 * 
 * Free block management - Usage of coalesce_mem() and then free()
 * Freed blocks are immediately merged with adjacent free blocks using boundary tags 
 * to minimize fragmentation. This coalescing process occurs both when a block is freed
 * and when the heap is extended. These blocks are then marked as free using the free()
 * function which then allows them to be used in realloc() or new malloc() calls.
 * 
 * Memory Reallocation - Usage of realloc()
 * The realloc function tries to expand the current block in place when
 * possible. If this is not feasible, it allocates a new block, transfers
 * the old data, and frees the previous block.
 * 
 * Heap error detection = Usage of mm_checkheap()
 * The heap checker code verifies essential heap properties to ensure integrity and 
 * correct alignment. It iterates over each block starting from heap_list_ptr and 
 * performs three main checks:
 *
 *  - Alignment Check: Ensures each block's starting address is correctly aligned according
 *    to the specified alignment requirement. If not, an error message is printed.
 *  - Size Validation: Verifies that each block's size meets the minimum alignment requirement
 *    and is a multiple of the alignment value, flagging any invalid sizes.
 *  - Header/Footer Consistency: Confirms that the header and footer of each block match in size
 *    and allocation status, detecting any mismatch between the two.
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
#define WORD_SIZE  8                   // Word and header/footer size
#define HEAP_EXTENSION 4096            // Extend heap by 4096 bytes
#define HEAP_MULTIPLIER 2              // Extend heap by this multiple of the requested size
#define MIN_BLOCK_SIZE 2               // Smallest possible size for a free block
#define MAX_LIST_POS  (ALIGNMENT - 1)  // Constant for highest position in the segregated list

// GLobal variables [TODO]
static char *heap_list_ptr;                                      // The first pointer to the heap block
static void remove_from_tree(void *block_ptr);                   // Removes a given pointer from the tree we are building
static void insert_to_tree(void *block_ptr, size_t block_size);  // Adds a given pointer to the tree we are building
void *free_list[ALIGNMENT];

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
    return (*(size_t*)(ptr));
}

// Write a value (block size and allocated bit) to the address pointed by ptr
static inline void write_word(void* ptr, size_t value) 
{
    // Store the value at the memory address pointed by ptr
    (*(size_t*)(ptr) = value);  
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
// Footer is located at block size minus ALIGNMENT (16 bytes)
static inline void* footer(const void* block_ptr) 
{
    return (void*)((char*)block_ptr + get_size(header(block_ptr)) - ALIGNMENT);
}

// Compute the address of the next block in the heap
// The next block starts right after the current block, based on the current block's size
static inline void* next_block(const void* block_ptr) 
{
    return (void*)((char*)block_ptr + get_size(header(block_ptr)));  // Add the current block size to get the next block
}

// Compute the address of the previous pointer stored in the block
static void* get_previous_pointer(void* block_ptr)
{
    // The address of the previous pointer is stored at the start of the block
    return (char *)(block_ptr);
}

// Compute the address of the next pointer stored in the block
static void* get_next_pointer(void* block_ptr)
{
    // The address of the next pointer is stored at WORD_SIZE bytes offset from the block
    return ((char *)(block_ptr) + WORD_SIZE);
}

// Dereference the pointer stored at the previous pointer location
static void* get_previous_block(void* block_ptr)
{
    // Dereference the address to get the actual previous block pointer
    return (*(char **)(block_ptr));
}

// Dereference the pointer stored at the next pointer location
static void* get_next_block(void* block_ptr)
{
    // Get the next pointer using get_next_pointer, then dereference it to get the next block
    return (*(char **)(get_next_pointer(block_ptr)));
}

// Set the value of a pointer (stored in a block)
static void set_block_pointer(void* block_ptr, void* target_ptr)
{
    // Set the pointer at the specified location to point to target_ptr
    (*(uint64_t* )(block_ptr) = (uint64_t)(target_ptr));
}

// Compute the address of the previous block in the heap
// The previous block ends just before the current block
// Move backward by the size of the previous block to locate its start
static inline void* prev_block(const void* block_ptr) 
{
    return (void*)((char*)block_ptr - get_size((char*)block_ptr - ALIGNMENT));  
}

/* rounds up to the nearest multiple of ALIGNMENT */
static size_t align(size_t x)
{
    return ALIGNMENT * ((x+ALIGNMENT-1)/ALIGNMENT);
}

// Find the smaller size of the size_t arguments
static size_t smaller_blk_size(size_t x, size_t y)
{
    if (x > y)
    {
        return x;
    }
    else
    {
        return y;
    }
}

// Find the bigger size of the size_t arguments
static inline size_t bigger_blk_size(size_t x, size_t y)
{
    if (x < y)
    {
        return x;
    } else
    {
        return y;
    }
}


// Function to select the appropriate size class [TODO for checkpoint 2]
int get_size_class(size_t size)
{
    if (size <= 32) return 0;         // Class 0: 32 bytes or less
    else if (size <= 64) return 1;    // Class 1: 33-64 bytes
    else if (size <= 128) return 2;   // Class 2: 65-128 bytes
    else if (size <= 256) return 3;   // Class 3: 129-256 bytes
    else if (size <= 512) return 4;   // Class 4: 257-512 bytes
    else if (size <= 1024) return 5;  // Class 5: 513-1024 bytes
    else if (size <= 2048) return 6;  // Class 6: 1025-2048 bytes
    else if (size <= 4096) return 7;  // Class 7: 2049-4096 bytes
    else if (size <= 8192) return 8;  // Class 8: 4097-8192 bytes
    else return 9;                    // Class 9: Above 8192 bytes
}

/*
 * Search for the first free block that fits the requested size
 */
static void* mem_block_size(size_t required_size)
{
    // Variables for tracking the current free list and block size
    size_t block_size = required_size;      // Initially, the required size for the block
    int header_position = 0;                // Start from the first free list

    // Iterate through the segregated free lists to find a suitable block
    while (header_position < ALIGNMENT) 
    {
        // Check if the current free list is not empty and block size is acceptable
        if (free_list[header_position] != NULL && block_size <= 1) 
        {
            void *block_ptr = NULL;
            block_ptr = free_list[header_position];

        // Traverse the free list to find a block large enough for the requested size
        for (bool block_init = true; block_ptr != NULL; block_ptr = get_previous_block(block_ptr)) 
        {
            if (required_size <= get_size(header(block_ptr))) 
            {
                // Suitable block found, return immediately
                return block_ptr;
            }
            // Print or log something for the first iteration only
            if (block_init != false) 
            {
                // First block for logging 
                // printf("First iteration of the free list\n");
                block_init = false;  // After first iteration, set it to false
            }
        }
            // If the block size is sufficient for the requested size, return the block
            if (block_size >= required_size) 
            {
                return block_ptr; // Suitable block found, return it
            }
        }

        // Move to the next free list and halve the size class for the next iteration
        block_size = block_size / 2;  // Reduce the block size to check the next size class
        header_position += 1;   // Move to the next list position
    }

    // If no suitable block is found, return NULL
    return NULL;
}

/*
 * Place the requested block at the start of the free block
 * and split the block if the remainder is large enough.
 */
static void *place(void *block_ptr, size_t adjusted_size) {
    // Get the current size of the block
    size_t current_size = get_size(header(block_ptr));

    // Calculate the remaining size after allocating the requested size
    size_t remaining_size = current_size - adjusted_size;

    // Remove the block from the free list as we are about to allocate it
    remove_from_tree(block_ptr);

    // Check if the remaining space is large enough to split the block
    if (remaining_size >= (2 * ALIGNMENT)) 
    {
        // Allocate the requested size in the current block
        write_word(header(block_ptr), pack(adjusted_size, 1));   // Mark header as allocated
        write_word(footer(block_ptr), pack(adjusted_size, 1));   // Mark footer as allocated

        // Calculate the location for the remaining free block
        void *free_block_ptr = next_block(block_ptr);

        // Set the header and footer for the new free block
        write_word(header(free_block_ptr), pack(remaining_size, 0));  // Free block header
        write_word(footer(free_block_ptr), pack(remaining_size, 0));  // Free block footer

        // Insert the new free block into the free list
        insert_to_tree(free_block_ptr, remaining_size);
    } else
    {
        // Allocate the entire block without splitting
        write_word(header(block_ptr), pack(current_size, 1));   // Mark the entire block as allocated
        write_word(footer(block_ptr), pack(current_size, 1));   // Mark footer accordingly
    }

    // Return the allocated block pointer
    return block_ptr;
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

    // Get allocation status of neighboring blocks
    size_t prev_alloc = get_alloc(footer(prev_block(block_ptr)));
    size_t next_alloc = get_alloc(header(next_block(block_ptr)));

    // Case 1: No coalescing needed if both neighbors are allocated
    if (prev_alloc && next_alloc) {
        return block_ptr;
    }

    // Remove the current block from the free list
    remove_from_tree(block_ptr);

    // Coalesce with the next block (ONLY IFnext block is free)
    if (!next_alloc) {
        size_t next_block_size = get_size(header(next_block(block_ptr)));
        size_t block_size = get_size(header(block_ptr)) + next_block_size;

        remove_from_tree(next_block(block_ptr));
        write_word(header(block_ptr), pack(block_size, 0));  // Update header
        write_word(footer(block_ptr), pack(block_size, 0));  // Update footer
    }

    // Coalesce with the previous block (ONLY IF previous block is free)
    if (!prev_alloc) {
        // Store the previous block pointer to avoid recalculating it multiple times
        void *prev_block_ptr = prev_block(block_ptr);

        // Calculate the combined block size by getting the sizes of the current and previous blocks
        size_t prev_block_size = get_size(header(prev_block_ptr));
        size_t block_size = get_size(header(block_ptr)) + prev_block_size;

        remove_from_tree(prev_block(block_ptr));
        write_word(footer(block_ptr), pack(block_size, 0));              // Update footer
        write_word(header(prev_block(block_ptr)), pack(block_size, 0));  // Update previous block header

        block_ptr = prev_block(block_ptr);  // Move block pointer to previous block
    }

    // Insert the coalesced block back into the free list
    insert_to_tree(block_ptr, get_size(header(block_ptr)));

    return block_ptr;  // Return the pointer to the coalesced block
}


/*
 * Implement a way to extend the size of the heap with a new free block
*/
static void *extend_heap(size_t words) {

    // Align the requested size
    size_t aligned_size = align(words);

    // Try to allocate memory, handle failure explicitly
    void *block_ptr = mem_sbrk(aligned_size);

    // Check for a valid memory allocation (successful allocation returns a non-negative pointer)
    // If memory allocation fails, return NULL
    if (block_ptr == NULL) {
        return NULL;
    }
    // Initialize the header and footer for the newly allocated free block
    write_word(header(block_ptr), pack(aligned_size, 0));  // Set free block header
    write_word(footer(block_ptr), pack(aligned_size, 0));  // Set free block footer

    // Initialize the epilogue header for the next block
    write_word(header(next_block(block_ptr)), pack(0, 1));  // Set epilogue header

    // Insert the newly allocated block into the segregated free list
    insert_to_tree(block_ptr, aligned_size);

    // Coalesce the block with adjacent free blocks, if possible, and return
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

    // Initialize the segregated free list to NULL using memset
    memset(free_list, 0, sizeof(free_list));

    // Padding operation before alignment, adding pointless alignment check
    int extra_padding = WORD_SIZE;  
    if (extra_padding > 0) 
    {
        write_word(initial_heap, 0);  // Set alignment padding
    }

    // Prologue setup (allocated)
    char* prologue_ptr = (char*) initial_heap;
    write_word(prologue_ptr + WORD_SIZE, pack(ALIGNMENT, 1));  // Prologue header
    write_word(prologue_ptr + (2 * WORD_SIZE), pack(ALIGNMENT, 1));  // Prologue footer

    // Initialize the epilogue header
    write_word(prologue_ptr + (3 * WORD_SIZE), pack(0, 1));  // Epilogue header

    // Add padding for prologue_ptr -> heap_list_ptr
    prologue_ptr += WORD_SIZE;
    heap_list_ptr = prologue_ptr + WORD_SIZE;

    // Extend the heap with a free block
    void* block = extend_heap(HEAP_EXTENSION / WORD_SIZE);
    if (!block)
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
    // Ignore size 0 requests
    if (size == 0)
    {
        return NULL;
    }

    size_t adjusted_size;

    // Check for small sizes first and set to the minimum block size
    if (size <= ALIGNMENT)
    {
        adjusted_size = 2 * ALIGNMENT;
    }
    else
    {
        // Add overhead (header/footer) to the requested size
        size_t total_size = size + (2 * WORD_SIZE);

        // Align the total size to the nearest alignment boundary
        adjusted_size = align(total_size);
    }

    // Try to find a suitable block from the free list
    char* block_ptr = mem_block_size(adjusted_size);
    if (block_ptr)
    {
        place(block_ptr, adjusted_size);
        return block_ptr;
    }

    // No fit found, extend the heap by the larger of the requested or default chunk size
    size_t extend_size = smaller_blk_size(adjusted_size, HEAP_EXTENSION);
    block_ptr = extend_heap(extend_size);
    
    if (!block_ptr)
    {
        return NULL;  // Heap extension failed
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
    // Return immediately if the pointer is NULL
    if (!ptr)
    {
        return;
    }

    // Retrieve the size of the block using the header
    size_t size = get_size(header(ptr));

    // Mark the block as free by writing the header and footer
    write_word(header(ptr), pack(size, 0));  // Set the header as free
    write_word(footer(ptr), pack(size, 0));  // Set the footer as free

    // Insert the block back into the segregated free list
    insert_to_tree(ptr, size);

    // Coalesce the block with adjacent free blocks
    coalesce_mem(ptr);
}

/*
 * The plan here is to create a list that we can use to traverse
 * between various points to make sure we can 
 */
static void remove_from_tree(void *block_ptr) 
{
    // Get the size of the block and initialize the list position
    size_t adjusted_size = get_size(header(block_ptr));
    int header_position = 0;

    // Find the correct segregated list based on the block size
    while (adjusted_size >= MIN_BLOCK_SIZE && header_position < MAX_LIST_POS) {
        adjusted_size = adjusted_size / 2;
        header_position++;
    }

    // Get pointers to the neighboring blocks in the free list
    void *prev_ptr = get_previous_block(block_ptr);
    void *next_ptr = get_next_block(block_ptr);

    // Case 1: Block is in the middle or at the front of the list
    if (prev_ptr != NULL) {
        // Update the next pointer of the previous block
        set_block_pointer(get_next_pointer(prev_ptr), next_ptr);
        
        if (next_ptr != NULL) {
            // Block is in the middle: update the previous pointer of the next block
            set_block_pointer(get_previous_pointer(next_ptr), prev_ptr);
        } else {
            // Block is at the front: update the list head
            free_list[header_position] = prev_ptr;
        }
    } 
    // Case 2: Block is at the end or the only block in the list
    else {
        if (next_ptr == NULL) {
            // Block is the only node: clear the list head
            free_list[header_position] = NULL;
        } else {
            // Block is at the end: update the previous pointer of the next block
            set_block_pointer(get_previous_pointer(next_ptr), NULL);
        }
    }
}

/*
 * Insert a block into the appropriate segregated free list
 */
static void insert_to_tree(void *block_ptr, size_t block_size) {
    // Determine the appropriate list position based on block size
    int header_position = 0;
    while (block_size > 1 && header_position < MAX_LIST_POS) {
        block_size = block_size / 2;
        header_position++;
    }

    // Find the position to insert
    void *current_ptr = free_list[header_position];
    void *prev_ptr = NULL;

    // Traverse the list to find the appropriate insertion point
    while (current_ptr != NULL && block_size > get_size(header(current_ptr))) {
        prev_ptr = current_ptr;
        current_ptr = get_previous_block(current_ptr);
    }

    // Insert the block into the correct position
    set_block_pointer(get_previous_pointer(block_ptr), current_ptr);
    set_block_pointer(get_next_pointer(block_ptr), prev_ptr);

    // If inserting at the head of the list
    if (!prev_ptr) {
        free_list[header_position] = block_ptr;
    } else {
        // Otherwise, update the previous block's next pointer
        set_block_pointer(get_previous_pointer(prev_ptr), block_ptr);
    }

    // If there's a next block, update its previous pointer
    if (current_ptr != NULL) {
        set_block_pointer(get_next_pointer(current_ptr), block_ptr);
    }
}

/*
 * realloc: reallocates a block of memory
 */
void* realloc(void* oldptr, size_t size)
{
    // If oldptr is NULL, equivalent to malloc(size)
    if (!oldptr) 
    {
        return malloc(size);
    }

    // If size is 0, free the block and return NULL
    if (size == 0) 
    {
        free(oldptr);
        return NULL;
    }

    // Allocate a new block with the requested size
    void* mem_ptr = malloc(size);
    if (!mem_ptr) 
    {
        return NULL;
    }

    // Copy the old data to the new block
    size_t prev_allocation_size = get_size(header(oldptr));  // Get the size of the old block
    // Use the static min function for clarity
    size_t copy_size = bigger_blk_size(prev_allocation_size, size);

    // Use memcpy to copy old memory to the new block
    memcpy(mem_ptr, oldptr, copy_size);

    // Free the old block
    free(oldptr);

    return mem_ptr;  // Return the new block
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
    // Check alignment of each block
    void *block_ptr = heap_list_ptr;

    while (get_size(header(block_ptr)) > 0)
    {
        // Check if the block is aligned
        if (!aligned(block_ptr)) 
        {
            printf("Heap error at line %d: Block at %p is not aligned\n", lineno, block_ptr);
            return false;
        }

        // Check todolist: [Sourajit: add more if you can come up with more]
        // - Check if the block being used has a valid size {DONE}
        // - Check if footer and header in a block matches

        // Check if each block has a valid size
        size_t block_size = get_size(header(block_ptr));
        // This part of the condition ensures that the size of each block is at least ALIGNMENT bytes.
        if (block_size < ALIGNMENT || block_size % ALIGNMENT != 0)
        {
            printf("Heap error at line %d: Block at %p has invalid size.\n", lineno, block_ptr, block_size);
            return false;
        }
        
        // Check if each block's header matches the footer
        // Main idea: Basically checks if the size value stored in the block's header is the same 
        // as the size value stored in the block's footer.
        if (get_size(header(block_ptr)) != get_size(footer(block_ptr)) || 
            get_alloc(header(block_ptr)) != get_alloc(footer(block_ptr))) 
        {
            printf("Heap error at line %d: Block at %p has mismatched header/footer\n", lineno, block_ptr);
            return false;
        }
    }

    // Print a message if all checks passed
    // printf("Heap check at line %d: All checks passed\n", lineno);
#endif /* DEBUG */
    return true;
}
