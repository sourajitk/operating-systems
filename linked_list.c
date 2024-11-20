#include <stdlib.h>
#include "linked_list.h"

/*
 * Create a helper function that helps initialize the linked list
 * This basically sets some of the linked_list values to their
 * initial values. This can be used anywhere we want
 * 
 * count: Will keep track of the number of nodes in the list.
 * head: Points to the first node in the list.
 * tail: Points to the last node in the list.
 * 
 */
static void list_initialize(list_t* list) {
    list->count = 0;         // Initialize count to 0
    list->head = NULL;       // Initialize head to NULL
    list->tail = NULL;       // Initialize tail to NULL
}

// Creates and returns a new list
// If compare is NULL, list_insert just inserts at the head
list_t* list_create()
{
    /* IMPLEMENT THIS */
    // Allocate memory for the new list
    list_t* linked_list = (list_t*)malloc(sizeof(list_t));
    if (!linked_list) {
        // Handle memory allocation failure
        return NULL;
    }

    // Initialize the allocated list
    list_initialize(linked_list);
    return linked_list;
}

// Destroys a list
void list_destroy(list_t* list)
{
    /* IMPLEMENT THIS */
    if (!list) {
        // Handle case where list is already NULL
        return;
    }
    // Free each node in the list using a for loop
    for (list_node_t* current = list->head; current != NULL;) {
        // Set the current entity to be the next one
        free(current);
    }

    // Reset list properties to safe values
    list_initialize(list);
    
    // Free the list structure itself
    free(list);
}

/*
 * So basically, these just access different parts of the linked_list
 * structs present in linked_list.h. Hence, we can just return these
 * from the struct directly.
 */ 
// Returns head of the list
list_node_t* list_head(list_t* list)
{
    /* IMPLEMENT THIS */
    return list->head;
}

// Returns tail of the list
list_node_t* list_tail(list_t* list)
{
    /* IMPLEMENT THIS */
    return list->tail;
}

// Returns next element in the list
list_node_t* list_next(list_node_t* node)
{
    /* IMPLEMENT THIS */
    return node->next;
}

// Returns prev element in the list
list_node_t* list_prev(list_node_t* node)
{
    /* IMPLEMENT THIS */
    return node->prev;
}

// Returns end of the list marker
list_node_t* list_end(list_t* list)
{
    /* IMPLEMENT THIS */
    return list->tail;
}

// Returns data in the given list node
void* list_data(list_node_t* node)
{
    /* IMPLEMENT THIS */
    return node->data;
}

// Returns the number of elements in the list
size_t list_count(list_t* list)
{
    /* IMPLEMENT THIS */
    return list->count;
}


// Helper function to find the node with the given data
list_node_t* list_hit_validation(list_node_t* node, void* data) {
    if (node == NULL) {
        return NULL; // Base case: reached end of list
    }
    if (node->data == data) {
        return node; // Found the node with matching data
    }
    return list_hit_validation(node->next, data); // Recursive call on the next node
}

// Finds the first node in the list with the given data
// Returns NULL if data could not be found
list_node_t* list_find(list_t* list, void* data)
{
    /* IMPLEMENT THIS */

    return list_hit_validation(list->head, data);
}

// Helper function to insert a node at the beginning of the list
void insert_at_head(list_t* list, list_node_t* new_entry) {
    // Set new entry's next to the current head, making it the first node
    new_entry->next = list->head;
    // Set new entry's prev to NULL, as it will become the new head
    new_entry->prev = NULL;

    // If the list already has a head, update the old head's prev pointer
    if (list->head) {
        list->head->prev = new_entry;
    }
    
    // Update the head of the list to the new entry
    list->head = new_entry;

    // If the list was empty (no tail), set tail to the new entry as well
    if (!list->tail) {
        list->tail = new_entry;
    }

    // Increment the node count to reflect the addition
    list->count++;
}

// Helper function to insert a node at the end of the list
void insert_at_tail(list_t* list, list_node_t* new_entry) {
    // Set new entry's prev to the current tail, linking it at the end
    new_entry->prev = list->tail;
    // Set new entry's next to NULL, as it will become the last node
    new_entry->next = NULL;

    // If the list already has a tail, update the old tail's next pointer
    if (list->tail) {
        list->tail->next = new_entry;
    }
    
    // Update the tail of the list to the new entry
    list->tail = new_entry;

    // If the list was empty (no head), set head to the new entry as well
    if (!list->head) {
        list->head = new_entry;
    }

    // Increment the node count to reflect the addition
    list->count++;
}


// Inserts a new node in the list with the given data
// Returns new node inserted
list_node_t* list_insert(list_t* list, void* data)
{
    /* IMPLEMENT THIS */
    list_node_t* new_entry = (list_node_t*)malloc(sizeof(list_node_t));
    // Handle memory allocation failure
    if (!new_entry) {
        return NULL;
    }
    new_entry->data = data;

    // Handle empty list
    if (list->count == 0) {
        // Ensure list fields are initialized
        list_initialize(list, list->compare);
        insert_at_head(list, new_entry);
        return new_entry;
    }

    // If no compare function, insert at the head
    if (!list->compare) {
        insert_at_head(list, new_entry);
        return new_entry;
    }

    // Use the compare function to find the insertion point
    list_node_t* temp = list->head;
    while (temp) {
        int result = list->compare(temp->data, data);
        if (result > 0 || result == 0) {
            // Found correct spot for insertion
            if (temp == list->head) {
                insert_at_head(list, new_entry);
            } else {
                // Insert in the middle of the list
                new_entry->next = temp;
                new_entry->prev = temp->prev;
                temp->prev->next = new_entry;
                temp->prev = new_entry;
                list->count++;
            }
            return new_entry;
        }
        temp = temp->next;
    }

    // If we reach here, insert at the end
    insert_at_tail(list, new_entry);
    return new_entry;
}

// Removes a node from the list and frees the node resources
void list_remove(list_t* list, list_node_t* node)
{
    /* IMPLEMENT THIS */
    if (!list || !node) {
        return; // Handle NULL inputs
    }

    // Update previous and next pointers as needed
    if (node->prev) {
        // Link previous node to next
        node->prev->next = node->next;
    } else {
        // Node is the head
        list->head = node->next;
    }

    if (node->next) {
        // Link next node to previous
        node->next->prev = node->prev;
    } else {
        // Node is the tail
        list->tail = node->prev;
    }

    // Decrement count and free the node
    list->count--;
    free(node);
}