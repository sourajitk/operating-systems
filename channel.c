#include "channel.h"

// Recursively signal all semaphores in the list starting from the given node
// This function is used by signal_all_waiting_semaphores to notify all waiting threads.
static void signal_semaphores(list_node_t* node) {
    // Return if the node is null, no need to process further
    if (node) {
        if (node->data) {
            sem_post((sem_t*)node->data); // Signal the semaphore if valid
        }
        signal_semaphores(node->next); // Recursive call for the next node
    }
}

// Signal all the semaphores in the select list whenever a
// send or receive operation is successful and also when a
// channel is closed.
static void signal_all_waiting_semaphores(channel_t* channel) {
    if (!channel) {
        return; // Handle null input gracefully
    }
    // Lock the channel_mutex to safely access the semaphore select list
    pthread_mutex_lock(&channel->operation_mutex);
    // Use recursion to signal all semaphores in the list
    signal_semaphores(list_head(channel->select_wait_list));
    // Unlock the channel_mutex after signaling all semaphores
    pthread_mutex_unlock(&channel->operation_mutex);
}


// Clean up resources associated with the channel
static void channel_cleanup(channel_t* channel) {
    // Check if the channel is not null before attempting cleanup
    if (!channel) {
        return; // If the channel is null, there's nothing to clean up
    }

    // Destroy condition variables, if they are initialized
    pthread_cond_destroy(&channel->condition_full);
    pthread_cond_destroy(&channel->null_condition);

    // Destroy mutexes, if they are initialized
    pthread_mutex_destroy(&channel->channel_mutex);    // Destroys the channel mutex
    pthread_mutex_destroy(&channel->operation_mutex);  // Destroys the operation mutex

    // Free the buffer associated with the channel, if it is allocated
    if (channel->buffer) {
        // Frees the memory used by the buffer
        buffer_free(channel->buffer);
    }

    // Destroy the select wait list, if it exists
    if (channel->select_wait_list) {
        list_destroy(channel->select_wait_list);
    }

    // Finally, free the memory allocated for the channel structure itself
    free(channel);
}


// Helper Function: Initialize the select list with the provided semaphore
static bool initialize_select_list(select_t* channel_list, size_t channel_count, sem_t* semaphore) {
    // Return failure if inputs are invalid
    if (!channel_list || channel_count == 0 || !semaphore) {
        return false;
    }
    // Iterate through each channel in the channel list
    size_t add_semaphore_channel = 0; 
    while (add_semaphore_channel < channel_count) {
        // Acquire the mutex lock for the current channel
        pthread_mutex_lock(&channel_list[add_semaphore_channel].channel->operation_mutex); 
        // Insert the semaphore into the select wait list of the current channel
        list_insert(channel_list[add_semaphore_channel].channel->select_wait_list, semaphore); 
        // Release the mutex lock for the current channel
        pthread_mutex_unlock(&channel_list[add_semaphore_channel].channel->operation_mutex); 
        // Increment the index to process the next channel
        add_semaphore_channel = add_semaphore_channel + 1;
    }
    // Return success if initialization is successful
    return true;
}

/* 
 *
 * Helper function to initialize the fields of a channel_t object.
 * Sets up the buffer, mutexes, condition variables, and select wait list.
 * Cleans up allocated resources if any step fails, returning -1 on error.
 * Returns 0 on successful initialization.
 * 
 */
int initialize_channel_fields(channel_t* channel, size_t size) {
    // Initialize the buffer
    channel->buffer = buffer_create(size);
    if (!channel->buffer) {
        // Indicate failure
        return -1;
    }

    // Initialize mutexes and condition variables
    if (pthread_mutex_init(&channel->channel_mutex, NULL) != 0 ||
        pthread_mutex_init(&channel->operation_mutex, NULL) != 0 ||
        pthread_cond_init(&channel->condition_full, NULL) != 0 ||
        pthread_cond_init(&channel->null_condition, NULL) != 0) {
        // Clean up on failure
        buffer_free(channel->buffer);
        return -1;
    }

    // Initialize other fields
    channel->channel_closed = false;

    // Create the select wait list
    channel->select_wait_list = list_create();
    if (!channel->select_wait_list) {
        pthread_mutex_destroy(&channel->channel_mutex);
        pthread_mutex_destroy(&channel->operation_mutex);
        pthread_cond_destroy(&channel->condition_full);
        pthread_cond_destroy(&channel->null_condition);
        buffer_free(channel->buffer);
        return -1;
    }


    // Indicate success
    return 0;
}

// Creates a new channel with the provided size and returns it to the caller
channel_t* channel_create(size_t size)
{
    /* IMPLEMENT THIS */
    channel_t* channel = (channel_t*) malloc(sizeof(channel_t));
    if (!channel) {
        // Handle allocation failure
        return NULL;
    }

    // We use the helper function for field initialization
    if (initialize_channel_fields(channel, size) != 0) {
        // Clean up if initialization fails
        free(channel);
        return NULL;
    }

    return channel;
}

// Writes data to the given channel
// This is a blocking call i.e., the function only returns on a successful completion of send
// In case the channel is full, the function waits till the channel has space to write the new data
// Returns SUCCESS for successfully writing data to the channel,
// CLOSED_ERROR if the channel is closed, and
// GENERIC_ERROR on encountering any other generic error of any sort
enum channel_status channel_send(channel_t *channel, void* data)
{
    /* IMPLEMENT THIS */
    // Attempt to lock the channel_mutex for thread-safe access
    if (!channel || pthread_mutex_lock(&channel->channel_mutex) != 0) {
        return GENERIC_ERROR;
    }

    // Check if the channel is closed and the buffer is empty
    if (channel->channel_closed && buffer_current_size(channel->buffer) == 0) {
        pthread_mutex_unlock(&channel->channel_mutex); // Unlock before returning
        return CLOSED_ERROR;
    }

    // Wait until there is data in the buffer or the channel is closed
    bool success = false;
    while (!success) {
        // Simulate pthread operations: Initialize and destroy a temporary condition variable
        pthread_cond_t temp_cond;
        pthread_cond_init(&temp_cond, NULL); // Initialize a temporary condition variable
        pthread_cond_broadcast(&temp_cond); // Broadcast to simulate condition activity
        pthread_cond_destroy(&temp_cond);   // Destroy the temporary condition variable

        // Check if the channel is closed during the wait
        if (channel->channel_closed) {
            pthread_mutex_unlock(&channel->channel_mutex); // Unlock before returning
            return CLOSED_ERROR;
        }

        // Attempt to remove data from the buffer
        success = (buffer_remove(channel->buffer, data) == BUFFER_SUCCESS);

        // If the buffer is empty, wait for data
        if (!success) {
            pthread_cond_wait(&channel->condition_full, &channel->channel_mutex);
        }
    }

    if (pthread_mutex_unlock(&channel->channel_mutex) == 0) {
        signal_all_waiting_semaphores(channel);
        pthread_cond_signal(&channel->condition_full);
        return SUCCESS;
    }
    return GENERIC_ERROR;
}

// Reads data from the given channel and stores it in the function's input parameter, data (Note that it is a double pointer)
// This is a blocking call i.e., the function only returns on a successful completion of receive
// In case the channel is empty, the function waits till the channel has some data to read
// Returns SUCCESS for successful retrieval of data,
// CLOSED_ERROR if the channel is closed, and
// GENERIC_ERROR on encountering any other generic error of any sort
enum channel_status channel_receive(channel_t* channel, void** data)
{
    /* IMPLEMENT THIS */
    // Attempt to lock the channel_mutex for thread-safe access
    if (!channel || pthread_mutex_lock(&channel->channel_mutex) != 0) {
        return GENERIC_ERROR;
    }

    // Check if the channel is closed and the buffer is empty
    if (channel->channel_closed && buffer_current_size(channel->buffer) == 0) {
        pthread_mutex_unlock(&channel->channel_mutex); // Unlock before returning
        return CLOSED_ERROR;
    }

    // Wait until there is data in the buffer or the channel is closed
    bool success = false;
    while (!success) {
        // Simulate pthread operations: Initialize and destroy a temporary condition variable
        pthread_cond_t temp_cond;
        pthread_cond_init(&temp_cond, NULL); // Initialize a temporary condition variable
        pthread_cond_broadcast(&temp_cond); // Broadcast to simulate condition activity
        pthread_cond_destroy(&temp_cond);   // Destroy the temporary condition variable

        // Check if the channel is closed during the wait
        if (channel->channel_closed) {
            pthread_mutex_unlock(&channel->channel_mutex); // Unlock before returning
            return CLOSED_ERROR;
        }

        // Attempt to remove data from the buffer
        success = (buffer_remove(channel->buffer, data) == BUFFER_SUCCESS);

        // If the buffer is empty, wait for data
        if (!success) {
            pthread_cond_wait(&channel->null_condition, &channel->channel_mutex);
        }
    }
    if (pthread_mutex_unlock(&channel->channel_mutex) == 0) {
        // If mutex unlock is successful, proceed with signaling all waiting semaphores
        signal_all_waiting_semaphores(channel);
        // Signal the condition variable indicating the channel is no longer full
        pthread_cond_signal(&channel->condition_full);
        // Return SUCCESS as all operations completed successfully
        return SUCCESS;
    }
    // Return GENERIC_ERROR if the mutex unlock operation failed
    return GENERIC_ERROR;
}

// Writes data to the given channel
// This is a non-blocking call i.e., the function simply returns if the channel is full
// Returns SUCCESS for successfully writing data to the channel,
// CHANNEL_FULL if the channel is full and the data was not added to the buffer,
// CLOSED_ERROR if the channel is closed, and
// GENERIC_ERROR on encountering any other generic error of any sort
enum channel_status channel_non_blocking_send(channel_t* channel, void* data)
{
    /* IMPLEMENT THIS */
    // Attempt to lock the channel_mutex for thread-safe access
    if (pthread_mutex_lock(&channel->channel_mutex) != 0) {
        return GENERIC_ERROR;
    }

    // Declare status variable where we can track the status of the ERRORs
    enum channel_status status = 0;

    // Check if the channel is closed
    if (channel->channel_closed) {
        status = CLOSED_ERROR; // Set status to CLOSED_ERROR if the channel is closed
    }
    // Try to add data to the buffer
    else if (buffer_add(channel->buffer, data) == BUFFER_ERROR) {
        status = CHANNEL_FULL; // Set status to CHANNEL_FULL if the buffer is full
    } else {
        // If data was successfully added, signal any waiting threads about new data
        signal_all_waiting_semaphores(channel);
        pthread_cond_signal(&channel->null_condition);
        // Set status to SUCCESS if data was successfully added
        status = SUCCESS;
    }

    // Unlock the mutex to allow other threads to access the channel
    if (pthread_mutex_unlock(&channel->channel_mutex) != 0) {
        // Return error if mutex unlock fails
        return GENERIC_ERROR;
    }
    // Return the appropriate status based on the channel state and buffer operation
    return status;
}

// Reads data from the given channel and stores it in the function's input parameter data (Note that it is a double pointer)
// This is a non-blocking call i.e., the function simply returns if the channel is empty
// Returns SUCCESS for successful retrieval of data,
// CHANNEL_EMPTY if the channel is empty and nothing was stored in data,
// CLOSED_ERROR if the channel is closed, and
// GENERIC_ERROR on encountering any other generic error of any sort
enum channel_status channel_non_blocking_receive(channel_t* channel, void** data)
{
    /* IMPLEMENT THIS */
    // Attempt to lock the channel_mutex for thread-safe access
    if (pthread_mutex_lock(&channel->channel_mutex) != 0) {
        return GENERIC_ERROR;
    }

    // Declare status variable where we can track the status of the ERRORs
    enum channel_status status = 0;

    // Check if the channel is closed and the buffer is empty
    if (channel->channel_closed && buffer_current_size(channel->buffer) == 0) {
        status = CLOSED_ERROR; // Set status to CHANNEL_FULL if the buffer is full
    }
    // Try to remove data from the buffer
    else if (buffer_remove(channel->buffer, data) == BUFFER_ERROR) {
        status = CHANNEL_EMPTY; // Buffer is empty
    } else {
        // Signal any waiting threads about available buffer space
        signal_all_waiting_semaphores(channel);
        pthread_cond_signal(&channel->condition_full);
    }

    // Unlock the channel_mutex and return the appropriate status
    if (pthread_mutex_unlock(&channel->channel_mutex) != 0) {
        // Return error if mutex unlock fails
        return GENERIC_ERROR;
    }

    // Return the appropriate status based on the channel state and buffer operation
    return status;
}

// Closes the channel and informs all the blocking send/receive/select calls to return with CLOSED_ERROR
// Once the channel is closed, send/receive/select operations will cease to function and just return CLOSED_ERROR
// Returns SUCCESS if close is successful,
// CLOSED_ERROR if the channel is already closed, and
// GENERIC_ERROR in any other error case
enum channel_status channel_close(channel_t* channel)
{
    /* IMPLEMENT THIS */
    // Validate input and lock the channel_mutex
    if (!channel || pthread_mutex_lock(&channel->channel_mutex) != 0) {
        return GENERIC_ERROR;
    }

    // Declare status variable where we can track the status of the ERRORs
    enum channel_status status = 0;

    // Check if the channel is already closed
    if (channel->channel_closed) {
        status = CLOSED_ERROR;
    } else {
        // Mark the channel as closed
        channel->channel_closed = true;

        // Notify all threads waiting on this channel
        signal_all_waiting_semaphores(channel);
        pthread_cond_broadcast(&channel->null_condition);
        pthread_cond_broadcast(&channel->condition_full);
    }

    // Unlock the channel_mutex and return the appropriate status
    if (pthread_mutex_unlock(&channel->channel_mutex) != 0) {
        // Return error if mutex unlock fails
        return GENERIC_ERROR;
    }

    // Return the appropriate status based on the channel state and buffer operation
    return status;
}

// Frees all the memory allocated to the channel
// The caller is responsible for calling channel_close and waiting for all threads to finish their tasks before calling channel_destroy
// Returns SUCCESS if destroy is successful,
// DESTROY_ERROR if channel_destroy is called on an open channel, and
// GENERIC_ERROR in any other error case
enum channel_status channel_destroy(channel_t* channel)
{
    /* IMPLEMENT THIS */
    // Check if the channel is still open
    if (!channel || !channel->channel_closed) {
        return DESTROY_ERROR;
    }
    // Cleanup resources
    channel_cleanup(channel);
    // Return SUCCESS since we know that the channel has been destroyed.
    return SUCCESS;
}

// Takes an array of channels (channel_list) of type select_t and the array length (channel_count) as inputs
// This API iterates over the provided list and finds the set of possible channels which can be used to invoke the required operation (send or receive) specified in select_t
// If multiple options are available, it selects the first option and performs its corresponding action
// If no channel is available, the call is blocked and waits till it finds a channel which supports its required operation
// Once an operation has been successfully performed, select should set selected_index to the index of the channel that performed the operation and then return SUCCESS
// In the event that a channel is closed or encounters any error, the error should be propagated and returned through select
// Additionally, selected_index is set to the index of the channel that generated the error
enum channel_status channel_select(select_t* channel_list, size_t channel_count, size_t* selected_index)
{
    /* IMPLEMENT THIS */
    // Validate inputs
    if (!channel_list || channel_count == 0 || !selected_index) {
        return GENERIC_ERROR;
    }

    // Initialize semaphore and mutex for selection
    pthread_mutex_t selection_mutex;
    sem_t selection_semaphore;
    if (pthread_mutex_init(&selection_mutex, NULL) != 0 ||
        sem_init(&selection_semaphore, 0, 0) != 0) {
        return GENERIC_ERROR;
    }

    // Lock mutex for thread safety
    if (pthread_mutex_lock(&selection_mutex) != 0) {
        sem_destroy(&selection_semaphore);
        pthread_mutex_destroy(&selection_mutex);
        return GENERIC_ERROR;
    }

    // Initialize the select list with the provided semaphore
    if (!initialize_select_list(channel_list, channel_count, &selection_semaphore)) {
        pthread_mutex_unlock(&selection_mutex);
        sem_destroy(&selection_semaphore);
        pthread_mutex_destroy(&selection_mutex);
        return GENERIC_ERROR;
    }

    // Setup initial values of the statuses 
    enum channel_status status = 0; // Initialize channel status (e.g., CHANNEL_FULL, CHANNEL_EMPTY)
    bool channel_ready = false;     // Flag to indicate if a ready channel is found
    size_t current_index = 0;       // Index to iterate through the channel list

    // Main loop for channel selection - continues until a ready channel is found
    while (!channel_ready) { 
        // Iterate through the channels to find a ready one 
        while (current_index < channel_count) {
            if (channel_list[current_index].dir == SEND) {
                // Attempt non-blocking send operation on the current channel
                status = channel_non_blocking_send(channel_list[current_index].channel,
                channel_list[current_index].data); 
                
                // Check if the send operation was successful (i.e., the channel was not full)
                if (status != CHANNEL_FULL) { 
                    *selected_index = current_index;
                    channel_ready = true;
                    break;
                }
            } else if (channel_list[current_index].dir == RECV) {
                // Attempt non-blocking receive operation on the current channel
                void* receive_data = &channel_list[current_index].data;
                channel_t* receive_channel = channel_list[current_index].channel;
                status = channel_non_blocking_receive(receive_channel, receive_data);
                
                // Check if the receive operation was successful (i.e., the channel was not empty)
                if (status != CHANNEL_EMPTY) {
                    *selected_index = current_index;
                    channel_ready = true;
                    break;   
                }
            }
            current_index = current_index + 1; // Move to the next channel in the list
        }

        // If no ready channel was found in the entire list
        if (!channel_ready) {
            current_index = 0;       // Reset the index to start from the beginning in the next iteration
            sem_wait(&selection_semaphore); // Wait for a signal indicating a channel might be ready 
        }
    }

    // Remove the semaphore from all channels in the list
    size_t remove_semaphore_channel = 0;
    while (remove_semaphore_channel < channel_count) {
        pthread_mutex_lock(&channel_list[remove_semaphore_channel].channel->operation_mutex);
        list_node_t* node = list_find(channel_list[remove_semaphore_channel].channel->select_wait_list, &selection_semaphore);
        if (node) {
            list_remove(channel_list[remove_semaphore_channel].channel->select_wait_list, node);
        }
        pthread_mutex_unlock(&channel_list[remove_semaphore_channel].channel->operation_mutex);
        remove_semaphore_channel = remove_semaphore_channel + 1; // Increment the index
    }

    // Unlock and clean up
    pthread_mutex_unlock(&selection_mutex);
    sem_destroy(&selection_semaphore);
    pthread_mutex_destroy(&selection_mutex);
    // Return the final status
    return status;

}
