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
static size_t smaller_blk_size(size_t x, size_t y){
    if (x <= y) {
        return x;
    }
    else {
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

    // Only split if the remaining block size is considerably larger than needed
    if ((current_size - adjusted_size) >= (4 * ALIGNMENT))  // Larger threshold to avoid frequent splitting
    {
        write_word(header(block_ptr), pack(adjusted_size, 1));
        write_word(footer(block_ptr), pack(adjusted_size, 1));

        // Create a free block with the remaining space
        block_ptr = next_block(block_ptr);
        size_t remaining_size = current_size - adjusted_size;
        write_word(header(block_ptr), pack(remaining_size, 0));
        write_word(footer(block_ptr), pack(remaining_size, 0));
    }
    else
    {
        // Allocate the entire block if splitting isn't necessary
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
    size_t extend_size = smaller_blk_size(words * WORD_SIZE, 
    HEAP_EXTENSION * HEAP_EXTENSION);
    void* block_ptr = mem_sbrk(extend_size);

    if (block_ptr == (void*) -1)
    {
        return NULL;  // If memory allocation fails
    }

    // Initialize the free block and the epilogue header
    write_word(header(block_ptr), pack(extend_size, 0));  // Free block header
    write_word(footer(block_ptr), pack(extend_size, 0));  // Free block footer

    write_word(header(next_block(block_ptr)), pack(0, 1));  // New epilogue header

    return coalesce_mem(block_ptr);  // Coalesce if necessary
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

    // Get the head of the segregated free list for this size class
    void *tree_tip = free_list[header_position];

    // If the list is empty, insert block at the head
    if (tree_tip == NULL) {
        // Since this is the only block in the list, both pointers are NULL
        free_list[header_position] = block_ptr;
        
        // Set both the previous and next pointers of the block to NULL in one step
        set_block_pointer(get_previous_pointer(block_ptr), NULL);
        set_block_pointer(get_next_pointer(block_ptr), NULL);
        
        return;  // Exit after insertion
    }

    // Traverse the list and find the correct position to insert the block
    void *current_ptr = tree_tip;
    void *prev_ptr = NULL;

    while (current_ptr != NULL && get_size(header(current_ptr)) < get_size(header(block_ptr))) {
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
 * realloc
 */
void* realloc(void* oldptr, size_t size)
{
    /* IMPLEMENT THIS */
    // If pointer is NULL, directly allocate new memory (acts like malloc)
    if (!oldptr)
    {
        return malloc(size);
    }

    // If the size is 0, free the block and return NULL
    if (size == 0)
    {
        free(oldptr);
        return NULL;
    }

    // Try shrinking or extending in place if possible
    size_t old_size = get_size(header(oldptr));

    // If the new size is smaller or equal, shrink in place
    if (size <= old_size)
    {
        if (old_size - size >= 32)
        {
            // Split the block to free the remaining space
            size_t remaining_size = old_size - size;
            write_word(header(oldptr), pack(size, 1));  // Shrink the current block
            write_word(footer(oldptr), pack(size, 1));

            // Create a new free block in the remaining space
            void* new_free_block = next_block(oldptr);
            write_word(header(new_free_block), pack(remaining_size, 0));
            write_word(footer(new_free_block), pack(remaining_size, 0));
        }
        return oldptr;  // Return the same pointer if resized in place
    }

    // Extend if adjacent free block is available
    void* next_block_ptr = next_block(oldptr);
    if (!get_alloc(header(next_block_ptr)))
    {
        size_t next_block_size = get_size(header(next_block_ptr));
        if ((old_size + next_block_size) >= size)
        {
            // Merge the two blocks
            size_t total_size = old_size + next_block_size;
            write_word(header(oldptr), pack(total_size, 1));
            write_word(footer(oldptr), pack(total_size, 1));
            return oldptr;  // Resized in place by merging
        }
    }

    // Allocate new memory if in-place resizing is not possible
    void* newptr = malloc(size);
    if (!newptr)
    {
        return NULL;  // Allocation failed
    }

    // Copy data from old block to new block manually
    mem_memcpy(newptr, oldptr, old_size);

    // Free the old block after copying data
    free(oldptr);
    return newptr;
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
